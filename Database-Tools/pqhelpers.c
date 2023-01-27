/*
Copyright 2023 En-En-Code

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "pqhelpers.h"

inline PGconn* pqInitConnection(const char* conninfo) {
    PGconn*     conn;
    PGresult*   res;
    
    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "%s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }

    // Set always-secure search path, so malicious users can't take control.
    // Recommended code from https://www.postgresql.org/docs/15/libpq-example.html
    res = PQexec(conn,
                 "SELECT pg_catalog.set_config('search_path', '', false)");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        exit(2);
    }
    PQclear(res);
    
    //Set schema to engines to match the creation file
    res = PQexec(conn, "SET search_path TO engines;");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "SET failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        exit(1);
    }
    PQclear(res);
    
    return conn;
}

// If calling this function, it is assumed the result of the look-up
// was successful and res contains table data.
inline void pqPrintTable(PGresult* res) {
    // TODO: explore using PQprint for more robust output
    int nFields = PQnfields(res);
    for (int i = 0; i < nFields; i += 1) {
        printf("%-20s", PQfname(res, i));
    }
    printf("\n");
    for (int i = 0; i < PQntuples(res); i += 1) {
        for (int j = 0; j < nFields; j += 1) {
            printf("%-20s", PQgetvalue(res, i, j));
        }
        printf("\n");
    }
    printf("\n");
}

inline void pqListAllEngines(PGconn* conn) {
    PGresult* res;
    
    res = PQexec(conn,
        "SELECT engine_id, engine_name FROM engines;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    
    PQclear(res);
}

inline void pqListAllVersions(PGconn* conn, char* engine_name) {
    /*
    Note that it is neither necessary nor correct to do escaping when
    a data value is passed as a separate parameter in PQexecParams.
        -- https://www.postgresql.org/docs/15/libpq-exec.html#LIBPQ-EXEC-ESCAPE-STRING
    */
    const char* paramValues[1] = { engine_name };

    PGresult* res;
    /*
    Parameterized statements use stored queries that have markers, known as
    parameters, to represent the input data. Instead of parsing the query and
    the data as a single string, the database reads only the stored query as
    query language, allowing user inputs to be sent as a list of parameters
    that the database can treat solely as data.
        -- https://www.crunchydata.com/blog/preventing-sql-injection-attacks-in-postgresql
    */
    res = PQexecParams(conn,
                        "SELECT engine_name, version_num, release_date, "
                        "program_lang, license, accepts_xboard, accepts_uci, notes "
                        "FROM versions v JOIN engines e ON v.engine_id = e.engine_id AND engine_name = $1;",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    
    PQclear(res);
}

// On success, returns the engine_id of the engine just inserted, and -1 on failure.
inline int pqAddNewEngine(PGconn* conn, char* engine_name) {
    const char* paramValues[1] = { engine_name };
    
    PGresult* res;
    res = PQexecParams(conn,
                        "INSERT INTO engines (engine_name) VALUES ($1)"
                        "RETURNING engine_id;", // PostgreSQL extension RETURNING
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    int ret = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    
    return ret;
}

// A helper function for inserting a newline-delimited series of strings into a database table
inline int pqAddNewNDSeries(PGconn* conn, int engine_id, char* nd_series, char** literals) {
    char itoc_str[25];
    snprintf(itoc_str, 25, "%d", engine_id);
    int ret = 0;
    
    int start_loc = 0;
    int end_loc = strcspn((nd_series + start_loc), "\n");
    while (start_loc != end_loc) {
        *(nd_series + end_loc) = '\0';
        if (pqAddNewElement(conn, itoc_str, (nd_series + start_loc), literals) == -1) {
            ret = -1;
        }
        start_loc = end_loc + 1;
        end_loc = start_loc + strcspn((nd_series + start_loc), "\n");
    }
    
    return ret;
}

// A helper function for inserting individual string elements into a database table
inline int pqAddNewElement(PGconn* conn, char* itoc_str, char* element, char** literals) {
    const char* paramValues[2] = { itoc_str, element };
    char query_maker[256];
    snprintf(query_maker, 256, "INSERT INTO %s (engine_id, %s) VALUES ($1, $2);",
                literals[0], literals[1]);
    const char* query = query_maker;
    
    PGresult* res;
    res = PQexecParams(conn, query, 2, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    
    return 0;
}

inline int pqAddNewNDAuthors(PGconn* conn, int engine_id, char* authors) {
    char* literals[2] = {"authors", "author_name"};
    return pqAddNewNDSeries(conn, engine_id, authors, literals);
}

inline int pqAddNewNDSources(PGconn* conn, int engine_id, char* sources) {
    char* literals[2] = {"sources", "code_link"};
    return pqAddNewNDSeries(conn, engine_id, sources, literals);
}

inline int pqAddNewVersion(PGconn* conn, int engine_id, version version_info) {
    char itoc_str[25];
    snprintf(itoc_str, 25, "%d", engine_id);
    // Year-MM-DD, the most sane format
    char itodate[40];
    snprintf(itodate, 40, "%d-%d-%d",
            version_info.releaseDate[0], version_info.releaseDate[1], version_info.releaseDate[2]);
    
    const char* paramValues[8] = {
        itoc_str,
        version_info.versionNum,
        itodate,
        version_info.programLang,
        version_info.license,
        (version_info.protocol & 1) ? "TRUE" : "FALSE",
        (version_info.protocol & 2) ? "TRUE" : "FALSE",
        version_info.notes
    };
    
    PGresult* res;
    res = PQexecParams(conn,
                        "INSERT INTO versions (engine_id, version_num, release_date, "
                        "program_lang, license, accepts_xboard, accepts_uci, notes) "
                        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8)",
                        8, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}
