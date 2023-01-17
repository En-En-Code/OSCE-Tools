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

inline PGconn* initConnection(const char* conninfo) {
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
inline void printTable(PGresult* res) {
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

inline void listAllEngines(PGconn* conn) {
    PGresult* res;
    
    res = PQexec(conn,
        "SELECT engine_id, engine_name FROM engines;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    printTable(res);
    
    PQclear(res);
}

inline void listAllVersions(PGconn* conn, char* engine_name) {
    const char* paramValues[1];
    // Note that it is neither necessary nor correct to do escaping when
    // a data value is passed as a separate parameter in PQexecParams.
    // https://www.postgresql.org/docs/15/libpq-exec.html#LIBPQ-PQESCAPELITERAL
    // I still feel this really should have some kind of SQL injection protection.
    paramValues[0] = engine_name;

    PGresult* res;
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
    printTable(res);
    
    PQclear(res);
}
