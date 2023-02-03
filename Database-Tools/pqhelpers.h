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
extern int* pqAllocEngineIDsWithName(PGconn* conn, char* engine_name);
extern void pqListEnginesWithName(PGconn* conn, char* engine_name);

extern void pqListAllVersions(PGconn* conn, char* engine_name);

extern int  pqAddNewEngine(PGconn* conn, char* engine_name, char* note);
extern int  pqAddNewNDSeries(PGconn* conn, int engine_id, char* nd_series, char** literals);
extern int  pqAddNewElement(PGconn* conn, char* itoc_str, char* element, char** literals);
extern int  pqAddNewNDAuthors(PGconn* conn, int engine_id, char* authors);
extern int  pqAddNewNDSources(PGconn* conn, int engine_id, char* sources);
extern int  pqAddNewVersion(PGconn* conn, int engine_id, version version_info);
