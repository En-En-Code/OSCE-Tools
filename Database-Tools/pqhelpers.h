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

#include "globals.h"

extern PGconn* pqInitConnection(const char* conninfo);
extern void pqPrintTable(PGresult* res);

extern void pqListEngines(PGconn* conn);
extern int* pqAllocEngineIdsWithName(PGconn* conn, char* engine_name);
extern void pqListEnginesWithName(PGconn* conn, char* engine_name);

extern void     pqListAuthors(PGconn* conn, char* engine_id);
extern void     pqListSources(PGconn* conn, char* engine_id);
extern void     pqListVersions(PGconn* conn, char* engine_id);
extern char*    pqAllocLatestVersionDate(PGconn* conn, char* engine_id);

extern char*    pqAddNewEngine(PGconn* conn, char* engine_name, char* note);

extern int  pqAddNewNDSeries(PGconn* conn, char* engine_id, char* nd_series, const char** literals);

extern int  pqGetElementId(PGconn* conn, char* element, const char** literals, int insert_on_fail);
extern int  pqAddRelation(PGconn* conn, char* engine_id, int element_id, const char** literals);

extern int  pqAddNewNDAuthors(PGconn* conn, char* engine_id, char* authors);

extern int  pqAddNewSource(PGconn* conn, char* engine_id, code_link source);
extern int  pqAddNewVersion(PGconn* conn, char* engine_id, version version_info);
extern int  pqAddNewInspiration(PGconn* conn, char* engine_id, int parent_engine_id);
extern int  pqAddNewDerivative(PGconn* conn, char* engine_id, int parent_engine_id);

extern PGresult*  pqAllocAllSources(PGconn* conn);