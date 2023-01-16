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

inline void listAllEngines(PGconn* conn) {
    PGresult* res;
    
    res = PQexec(conn, "SELECT * FROM engines;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }
    int nFields = PQnfields(res);
    for (int i = 0; i < nFields; i += 1) {
        printf("%-20s", PQfname(res, i));
    }
    printf("\n");
    for (int i = 0; i < PQntuples(res); i += 1) {
        for (int j = 0; j < nFields; j++) {
            printf("%-20s", PQgetvalue(res, i, j));
        }
        printf("\n");
    }
    printf("\n");
    PQclear(res);
}
