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
#include <time.h>
#include <libpq-fe.h>
#include "pqhelpers.h"
#include "pkghelpers.h"
#include "globals.h"

PGconn* pqInitConnection(const char* conninfo) {
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
    res = PQexec(conn, "SET search_path TO engine;");
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
void pqPrintTable(PGresult* res) {
    // Print the table in a format similar to JSON or Rust's debug print
    printf("[");
    for (int i = 0; i < PQntuples(res); i += 1) {
        printf("\n");
        for (int j = 0; j < PQnfields(res); j += 1) {
            printf("  %-15s: %s\n", PQfname(res, j), PQgetvalue(res, i, j));
        }
    }
    printf("]\n");
}

void pqListEngines(PGconn* conn) {
    PGresult* res;
    
    res = PQexec(conn, "SELECT engine_name, note FROM engine ORDER BY engine_name ASC;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    
    PQclear(res);
}

// This function allocates an integer array, which needs to be freed when done.
int* pqAllocEngineIdsWithName(PGconn* conn, char* engine_name) {
    const char* paramValues[1] = { engine_name };
    PGresult* res;
    
    res = PQexecParams(conn,
        "SELECT engine_id FROM engine WHERE engine_name = $1;",
        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    // The first value in results stores the number of results, stored in the remaining values.
    int* results = (int*)errhandMalloc((PQntuples(res) + 1) * sizeof(int));
    results[0] = PQntuples(res);
    for (int i = 0; i < *(results); i += 1) {
        results[i+1] = atoi(PQgetvalue(res, i, 0));
    }
    
    PQclear(res);
    return results;
}

// This function allocates a char* on success, which needs to be freed when done.
char* pqAllocVersionIdWithName(PGconn* conn, char* engine_id, char* version_name) {
    const char* paramValues[2] = { engine_id, version_name };
    PGresult* res;

    res = PQexecParams(conn,
                        "SELECT version_id FROM version "
                        "WHERE engine_id = $1 AND version_name = $2;",
                        2, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    if (PQntuples(res)) {
        char* ret = errhandStrdup(PQgetvalue(res, 0, 0));
        PQclear(res);
        return ret;
    }

    PQclear(res);
    return NULL;
}

void pqListEnginesWithName(PGconn* conn, char* engine_name) {
    const char* paramValues[1] = { engine_name };
    PGresult* res;
    
    // I could maybe insert authors as well, but forming a Cartesian product seems annoying.
    res = PQexecParams(conn,
        "SELECT e.engine_id, engine_name, note, source_uri "
        "FROM (SELECT * FROM engine_source JOIN source USING (source_id)) temp "
        "RIGHT OUTER JOIN engine e USING (engine_id) WHERE engine_name = $1;",
        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    
    PQclear(res);
}

void pqListNote(PGconn* conn, char* engine_id) {
    const char* paramValues[1] = { engine_id };

    PGresult* res;
    res = PQexecParams(conn,
                        "SELECT note FROM engine WHERE engine_id = $1;",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    
    PQclear(res);
}

void pqListAuthors(PGconn* conn, char* engine_id) {
    const char* paramValues[1] = { engine_id };
    
    PGresult* res;
    res = PQexecParams(conn,
                        "SELECT author_name FROM author "
                        "JOIN engine_author USING (author_id) WHERE engine_id = $1",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    
    PQclear(res);
}

void pqListSources(PGconn* conn, char* engine_id) {
    const char* paramValues[1] = { engine_id };
    
    PGresult* res;
    res = PQexecParams(conn,
                        "SELECT source_uri, vcs_name FROM source JOIN vcs USING (vcs_id) "
                        "JOIN engine_source USING (source_id) WHERE engine_id = $1",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    
    PQclear(res);
}

void pqListVersions(PGconn* conn, char* engine_id) {
    /*
    Note that it is neither necessary nor correct to do escaping when
    a data value is passed as a separate parameter in PQexecParams.
        -- https://www.postgresql.org/docs/15/libpq-exec.html#LIBPQ-EXEC-ESCAPE-STRING
    */
    const char* paramValues[1] = { engine_id };

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
                        "SELECT version_name, source_uri, frag_type, frag_val, release_date, "
                        "code_lang_name, license_name, is_xboard, is_uci, v.note "
                        "FROM version v JOIN revision USING (revision_id) "
                        "JOIN source USING (source_id) JOIN engine USING (engine_id) "
                        "JOIN license USING (license_id) JOIN code_lang USING (code_lang_id) "
                        "WHERE v.engine_id = $1 ORDER BY release_date DESC;",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    
    PQclear(res);
}

void pqListVersionDetails(PGconn* conn, char* version_id) {
    const char* paramValues[1] = { version_id };

    PGresult* res;
    res = PQexecParams(conn,
                        "SELECT version_name, source_uri, frag_type, frag_val, release_date, "
                        "code_lang_name, license_name, is_xboard, is_uci, note FROM version v "
                        "JOIN revision USING (revision_id) JOIN source USING (source_id) "
                        "JOIN license USING (license_id) JOIN code_lang USING (code_lang_id) "
                        "WHERE version_id = $1 ORDER BY release_date DESC;",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    PQclear(res);

    res = PQexecParams(conn,
                        "SELECT os_name FROM version_os JOIN os USING (os_id) "
                        "WHERE version_id = $1;",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    PQclear(res);

    res = PQexecParams(conn,
                        "SELECT egtb_name FROM version_egtb JOIN egtb USING (egtb_id) "
                        "WHERE version_id = $1;",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    pqPrintTable(res);
    PQclear(res);
}

char* pqAllocLatestVersionDate(PGconn* conn, char* engine_id) {
    const char* paramValues[1] = { engine_id };

    PGresult* res;
    res = PQexecParams(conn,
                        "SELECT release_date FROM version v JOIN engine USING (engine_id) "
                        "JOIN license USING (license_id) "
                        "JOIN code_lang USING (code_lang_id) WHERE v.engine_id = $1 "
                        "ORDER BY release_date DESC LIMIT 1;",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    char* ret = errhandStrdup(PQgetvalue(res, 0, 0));

    PQclear(res);
    return ret;
}

// Returns the engine_id of the engine just inserted on success, NULL on failure.
char* pqInsertEngine(PGconn* conn, char* engine_name, char* note) {
    const char* paramValues[2] = { engine_name, note };
    
    PGresult* res;
    res = PQexecParams(conn,
                        "INSERT INTO engine (engine_name, note) VALUES ($1, $2) "
                        "RETURNING engine_id;", // PostgreSQL extension RETURNING
                        2, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    char* ret = errhandStrdup(PQgetvalue(res, 0, 0));

    PQclear(res);
    return ret;
}

// A helper function which searches the existing table for an entry.
// literals[0] contains the name of the relational table (e.g. engine_author, engine_source)
// literals[1] contains the name of the id table (e.g. author, source)
// literals[2] contains the name of the value in the id table (e.g. author_name, source_uri)
// If insert_on_fail is 0, not finding the element in the table returns a failure value.
// If insert_on_fail is 1, not finding the element results in the element being inserted.
// Returns the id associated with the object on success, -1 on failure.
int pqGetElementId(PGconn* conn, char* element, const char** literals, int insert_on_fail) {
    const char* paramValues[1] = { element };
    char query_maker[256];
    snprintf(query_maker, 256,
            "SELECT %s_id FROM %s WHERE %s = $1",
            literals[1], literals[1], literals[2]);
    const char* select_query = query_maker;
    
    PGresult* res;
    res = PQexecParams(conn, select_query, 1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    int ret = -1;
    if (PQntuples(res)) {
        ret = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);
        return ret;
    }
    PQclear(res);
    if (!insert_on_fail) {
        return -1;
    }
    // Since the string is not already in the table, it needs to be inserted.
    snprintf(query_maker, 256,
            "INSERT INTO %s (%s) VALUES ($1) RETURNING %s_id;",
            literals[1], literals[2], literals[1]);
    const char* insert_query = query_maker;
    
    res = PQexecParams(conn, insert_query, 1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    ret = atoi(PQgetvalue(res, 0, 0));

    PQclear(res);
    return ret;
}

// A helper function for inserting individual relations into a relational database table
// literals[0] contains the name of the relational table (e.g. engine_author, engine_source)
// literals[1] contains the name of the inserted object (e.g. author, source)
// literals[2] contains the name of the value in the id table (e.g. author_name, source_uri)
int pqAddRelation(PGconn* conn, char* engine_id, int element_id, const char** literals) {
    char ndidc_str[25];
    snprintf(ndidc_str, 25, "%d", element_id);
    const char* paramValues[2] = { engine_id, ndidc_str };
    char query_maker[256];
    snprintf(query_maker, 256, "INSERT INTO %s (engine_id, %s_id) VALUES ($1, $2);",
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

// Add a new source link if it does not exist, or retrieve its ID if it does.
// Then, add a relation between in engine_source behind an engine and the source.
int pqInsertSource(PGconn* conn, char* engine_id, code_link source) {
    const char* vcsParamValues[1] = { source.vcs };
    
    PGresult* res_vcs;
    res_vcs = PQexecParams(conn,
                            "SELECT vcs_id FROM vcs WHERE vcs_name = $1;",
                            1, NULL, vcsParamValues, NULL, NULL, 0);
    if (PQresultStatus(res_vcs) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res_vcs);
        return -1;
    }
    if (!PQntuples(res_vcs)) {
        fprintf(stderr, "%s is not a recognized version control system.\n", source.vcs);
        PQclear(res_vcs);
        return -1;
    }
    char* vcs = PQgetvalue(res_vcs, 0, 0);
    const char* sourceParamValues[2] = { source.uri, vcs };
    
    PGresult* res_source;
    res_source = PQexecParams(conn,
                                "INSERT INTO source (source_uri, vcs_id)"
                                "VALUES ($1, $2) RETURNING source_id;",
                                2, NULL, sourceParamValues, NULL, NULL, 0);
    if (PQresultStatus(res_source) != PGRES_TUPLES_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res_vcs);
        PQclear(res_source);
        return -1;
    }
    int source_id = atoi(PQgetvalue(res_source, 0, 0));
    
    const char* literals[2] = { "engine_source", "source" };
    int ret = pqAddRelation(conn, engine_id, source_id, literals);
    
    PQclear(res_vcs);
    PQclear(res_source);
    
    return ret;
}

int pqInsertAuthor(PGconn* conn, char* engine_id, char* author) {
    const char* author_literals[3] = { "engine_author", "author", "author_name" };
    return pqGetElementId(conn, author, author_literals, 1);
}

char* pqInsertVersion(PGconn* conn, char* engine_id, version version_info) {
    // Year-MM-DD, the most sane format
    char tmtodate[40];
    strftime(tmtodate, 40, "%Y-%m-%d", &version_info.releaseDate);
    
    const char* code_lang_literals[3] = {NULL, "code_lang", "code_lang_name"};
    int code_lang_id = pqGetElementId(conn, version_info.programLang, code_lang_literals, 0);
    if (code_lang_id == -1) {
        fprintf(stderr, "%s was not in the table and is not automatically inserted.\n", version_info.programLang);
        return NULL;
    }
    char code_lang_id_str[25];
    snprintf(code_lang_id_str, 25, "%d", code_lang_id);
    
    const char* license_literals[3] = {NULL, "license", "license_name"};
    int license_id = pqGetElementId(conn, version_info.license, license_literals, 0);
    if (license_id == -1) {
        fprintf(stderr, "%s was not in the table and is not automatically inserted.\n", version_info.license);
        return NULL;
    }
    char license_id_str[25];
    snprintf(license_id_str, 25, "%d", license_id);
    
    int revision_id = pqInsertRevision(conn, version_info.rev);
    if (revision_id == -1) {
        fprintf(stderr, "Revision data was not provided with the version.\n");
        return NULL;
    }
    char revision_id_str[25];
    snprintf(revision_id_str, 25, "%d", revision_id);

    const char* paramValues[9] = {
        engine_id,
        version_info.versionNum,
        revision_id_str,
        tmtodate,
        code_lang_id_str,
        license_id_str,
        (version_info.protocol & 1) ? "TRUE" : "FALSE",
        (version_info.protocol & 2) ? "TRUE" : "FALSE",
        version_info.note
    };
    
    PGresult* res;
    res = PQexecParams(conn,
                        "INSERT INTO version (engine_id, version_name, revision_id, "
                        "release_date, code_lang_id, license_id, is_xboard, is_uci, note) "
                        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) RETURNING version_id;",
                        9, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    char* ret = errhandStrdup(PQgetvalue(res, 0, 0));

    PQclear(res);
    return ret;
}

int pqInsertRevision(PGconn* conn, revision rev_info) {
    const char* paramValues[3] = {
        rev_info.code_id,
        (rev_info.type == 1) ? "branch" :
        (rev_info.type == 2) ? "commit" :
        (rev_info.type == 4) ? "revnum" : "tag",
        rev_info.val
    };
    
    PGresult* res;
    res = PQexecParams(conn,
                        "INSERT INTO revision (source_id, frag_type, frag_val)"
                        "VALUES ($1, $2, $3) RETURNING revision_id;",
                        3, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "INSERT failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    int ret = atoi(PQgetvalue(res, 0, 0));
    
    PQclear(res);
    return ret;
}

int pqInsertInspiration(PGconn* conn, char* engine_id, int origin_engine_id) {
    const char* literals[2] = { "inspiration", "origin_engine" };
    return pqAddRelation(conn, engine_id, origin_engine_id, literals);
}

int pqInsertPredecessor(PGconn* conn, char* engine_id, int origin_engine_id) {
    const char* literals[2] = { "predecessor", "origin_engine" };
    return pqAddRelation(conn, engine_id, origin_engine_id, literals);
}

int pqInsertVersionOs(PGconn* conn, char* version_id, char* os_name) {
    const char* os_literals[3] = {NULL, "os", "os_name"};
    int os_id = pqGetElementId(conn, os_name, os_literals, 0);
    if (os_id == -1) {
        fprintf(stderr, "%s was not in the table and is not automatically inserted.\n", os_name);
        return -1;
    }
    char os_id_str[25];
    snprintf(os_id_str, 25, "%d", os_id);

    const char* paramValues[2] = {version_id, os_id_str};
    PGresult* res;
    res = PQexecParams(conn,
                        "INSERT INTO version_os (version_id, os_id) VALUES ($1, $2);",
                        2, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int pqInsertVersionEgtb(PGconn* conn, char* version_id, char* egtb_name) {
    const char* egtb_literals[3] = {NULL, "egtb", "egtb_name"};
    int egtb_id = pqGetElementId(conn, egtb_name, egtb_literals, 0);
    if (egtb_id == -1) {
        fprintf(stderr, "%s was not in the table and is not automatically inserted.\n", egtb_name);
        return -1;
    }
    char egtb_id_str[25];
    snprintf(egtb_id_str, 25, "%d", egtb_id);

    const char* paramValues[2] = {version_id, egtb_id_str};
    PGresult* res;
    res = PQexecParams(conn,
                        "INSERT INTO version_egtb (version_id, egtb_id) VALUES ($1, $2);",
                        2, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

// Note: The caller is responsible for checking the query was successful and for freeing res.
PGresult* pqAllocAllBranchRevisions(PGconn* conn) {
    PGresult* res;
    res = PQexec(conn,
                "SELECT revision_id, source_uri, frag_type, frag_val, vcs_name, "
                "engine_id, source_id FROM revision JOIN source USING (source_id) "
                "JOIN vcs USING (vcs_id) JOIN engine_source USING (source_id) "
                "JOIN engine USING (engine_id) WHERE frag_type = 'branch';");
    return res;
}

code_link** pqAllocSourcesFromEngine(PGconn* conn, char* engine_id, size_t* dest_elems) {
    const char* paramValues[1] = { engine_id };

    PGresult* res;
    res = PQexecParams(conn,
                        "SELECT source_id, source_uri, vcs_name FROM source s "
                        "JOIN vcs USING (vcs_id) JOIN engine_source USING (source_id) "
                        "WHERE engine_id = $1;",
                        1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        dest_elems = 0;
        return NULL;
    }
    if (!PQntuples(res)) {
        fprintf(stderr, "No source links were found for this engine.\n");
        PQclear(res);
        dest_elems = 0;
        return NULL;
    }

    *dest_elems = PQntuples(res);
    code_link** sources = errhandMalloc(*dest_elems * sizeof(code_link*));
    for (int i = 0; i < *dest_elems; i++) {
        sources[i] = errhandMalloc(sizeof(code_link));
        sources[i]->id = errhandStrdup(PQgetvalue(res, i, 0));
        sources[i]->uri = errhandStrdup(PQgetvalue(res, i, 1));
        sources[i]->vcs = errhandStrdup(PQgetvalue(res, i, 2));
    }

    PQclear(res);
    return sources;
}

code_link* pqAllocSourceFromVersion(PGconn* conn, char* version_id) {
    const char* paramValues[1] = { version_id };

    PGresult* res;
    res = PQexecParams(conn,
                        "SELECT source_id, source_uri, vcs_name FROM version v "
                        "JOIN revision USING (revision_id) JOIN source USING (source_id) "
                        "JOIN vcs USING (vcs_id) WHERE version_id = $1;",
                       1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    code_link* source = errhandMalloc(sizeof(code_link));
    source->id = errhandStrdup(PQgetvalue(res, 0, 0));
    source->uri = errhandStrdup(PQgetvalue(res, 0, 1));
    source->vcs = errhandStrdup(PQgetvalue(res, 0, 2));

    PQclear(res);
    return source;
}

revision* pqAllocRevisionFromVersion(PGconn* conn, char* version_id) {
    const char* paramValues[1] = { version_id };
    
    PGresult* res;
    res = PQexecParams(conn,
                       "SELECT source_id, frag_type, frag_val FROM version v "
                       "JOIN revision USING (revision_id) WHERE version_id = $1",
                       1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    
    revision* rev = allocRevision(PQgetvalue(res, 0, 0), PQgetvalue(res, 0, 1),
                                  PQgetvalue(res, 0, 2), PQgetisnull(res, 0, 2));
    PQclear(res);
    return rev;
}

// Returns 0 on success of creating the table, and -1 on failure
int pqCreateUpdateTable(PGconn* conn) {
    PGresult* res;
    res = PQexec(conn, "CREATE TABLE update "
                       "(revision_id int REFERENCES revision (revision_id));");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "CREATE TABLE failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

// Returns 0 on success of creating the table, and -1 on failure
int pqInsertUpdate(PGconn* conn, char* revision_id) {
    const char* paramValues[1] = { revision_id };

    PGresult* res;
    res = PQexecParams(conn, "INSERT INTO update (revision_id) VALUES ($1);",
                            1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

void pqSummarizeUpdateTable(PGconn* conn) {
    PGresult* res;
    res = PQexec(conn, "SELECT engine_name, source_uri, vcs_name, version_name FROM engine_source "
                        "JOIN engine USING (engine_id) JOIN source USING (source_id) "
                        "JOIN vcs USING (vcs_id) JOIN revision USING (source_id) "
                        "JOIN update USING (revision_id) JOIN version USING (revision_id) "
                        "ORDER BY engine_name ASC;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    int i = 0;
    int tuples = PQntuples(res);
    while (i < tuples) {
        char* vcs = PQgetvalue(res, i, 2);
        if (strncmp(vcs, "git", 3) == 0 || strncmp(vcs, "svn", 3) == 0 || strncmp(vcs, "cvs", 3) == 0) {
            printf("%s %s has updates at %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 3), PQgetvalue(res, i, 1));
        } else if (strncmp(vcs, "rhv", 3) == 0) {
            printf("%s may have updates; check manually.\n", PQgetvalue(res, i, 0));
        }
        i += 1;
    }

    PQclear(res);
}

// Returns 0 on success of creating the table, and -1 on failure
int pqDropUpdateTable(PGconn* conn) {
    PGresult* res;
    res = PQexec(conn, "DROP TABLE update;");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "DROP TABLE failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

int pqUpdateVersionDate(PGconn* conn, char* version_id, struct tm* date) {
    char tmtodate[40];
    strftime(tmtodate, 40, "%Y-%m-%d", date);

    const char* paramValues[2] = { tmtodate, version_id };

    PGresult* res;
    res = PQexecParams(conn,
                        "UPDATE version SET release_date = $1 WHERE version_id = $2;",
                        2, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "UPDATE failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

int pqUpdateVersionNote(PGconn* conn, char* version_id, char* note) {
    const char* paramValues[2] = { note, version_id };

    PGresult* res;
    res = PQexecParams(conn,
                        "UPDATE version SET note = $1 WHERE version_id = $2;",
                        2, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "UPDATE failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

size_t pqExtractPkgbuild(PGconn* conn, char* version_id) {
    const char* paramValues[1] = { version_id };

    PGresult* res;
    res = PQexecParams(conn,
                       "SELECT pkgbuild FROM version WHERE version_id = $1;",
                       1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return 0;
    }
    size_t bytes = pkgStoreStringToFile(PQgetvalue(res, 0, 0));
    if (!bytes) {
        printf("No PKGBUILD stored in the database. Generating a default...\n");
        PQclear(res);
        res = PQexecParams(conn,
                           "SELECT engine_name, version_name, e.note, source_uri, "
                           "license_name, vcs_name, frag_type, frag_val FROM engine e "
                           "JOIN version USING (engine_id) JOIN revision USING (revision_id) "
                           "JOIN source USING (source_id) JOIN vcs USING (vcs_id) "
                           "JOIN license USING (license_id) WHERE version_id = $1;",
                           1, NULL, paramValues, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
            PQclear(res);
            return 0;
        }
        bytes = pkgCreateDefaultFile(PQgetvalue(res, 0, 0), PQgetvalue(res, 0, 1),
                                     PQgetvalue(res, 0, 2), PQgetvalue(res, 0, 3),
                                     PQgetvalue(res, 0, 4), PQgetvalue(res, 0, 5),
                                     PQgetvalue(res, 0, 6), PQgetvalue(res, 0, 7));
    }
    printf("%li bytes written to PKGBUILD.\n", bytes);

    PQclear(res);
    return bytes;
}

size_t pqUpdatePkgbuild(PGconn* conn, char* version_id) {
    char* pkgbuild = pkgAllocStringFromFile();
    if (!pkgbuild) {
        return -1;
    }

    const char* paramValues[2] = { pkgbuild, version_id };

    PGresult* res;
    res = PQexecParams(conn,
                       "UPDATE version SET pkgbuild = $1 WHERE version_id = $2;",
                       2, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "UPDATE failed: %s", PQerrorMessage(conn));
        PQclear(res);
        free(pkgbuild);
        return -1;
    }
    size_t len = strlen(pkgbuild);
    printf("%lu bytes written from PKGBUILD to database.\n", len);
    free(pkgbuild);

    return len;
}
