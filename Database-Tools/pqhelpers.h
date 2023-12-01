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

#ifndef PQHELPERS_H
#define PQHELPERS_H

#include "globals.h"

extern PGconn* pqInitConnection(const char* conninfo);

extern void pqPrintTable(PGresult* res);

extern void     pqListEngines(PGconn* conn);
extern int*     pqAllocEngineIdsWithName(PGconn* conn, char* engine_name);
extern char*    pqAllocVersionIdWithName(PGconn* conn, char* engine_id, char* version_name);
extern void     pqListEnginesWithName(PGconn* conn, char* engine_name);

extern void     pqListNote(PGconn* conn, char* engine_id);
extern void     pqListAuthors(PGconn* conn, char* engine_id);
extern void     pqListSources(PGconn* conn, char* engine_id);
extern void     pqListVersions(PGconn* conn, char* engine_id);
extern void     pqListVersionDetails(PGconn* conn, char* version_id);
extern char*    pqAllocLatestVersionDate(PGconn* conn, char* engine_id);

extern char*    pqInsertEngine(PGconn* conn, char* engine_name, char* note);

extern int  pqGetElementId(PGconn* conn, char* element, const char** literals, int insert_on_fail);
extern int  pqAddRelation(PGconn* conn, char* engine_id, int element_id, const char** literals);

extern int      pqInsertSource(PGconn* conn, char* engine_id, code_link source);
extern int      pqInsertAuthor(PGconn* conn, char* engine_id, char* author);
extern char*    pqInsertVersion(PGconn* conn, char* engine_id, version version_info);
extern int      pqInsertRevision(PGconn* conn, revision rev_info);
extern int      pqInsertInspiration(PGconn* conn, char* engine_id, int parent_engine_id);
extern int      pqInsertPredecessor(PGconn* conn, char* engine_id, int parent_engine_id);
extern int      pqInsertVersionOs(PGconn* conn, char* version_id, char* os_name);
extern int      pqInsertVersionEgtb(PGconn* conn, char* version_id, char* egtb_name);

extern PGresult*    pqAllocAllBranchRevisions(PGconn* conn);
extern code_link**  pqAllocSourcesFromEngine(PGconn* conn, char* engine_id, size_t* dest_elems);
extern code_link*   pqAllocSourceFromVersion(PGconn* conn, char* version_id);
extern revision*    pqAllocRevisionFromVersion(PGconn* conn, char* revision_id);

extern int  pqCreateUpdateTable(PGconn* conn);
extern int  pqInsertUpdate(PGconn* conn, char* revision_id);
extern void pqSummarizeUpdateTable(PGconn* conn);
extern int  pqDropUpdateTable(PGconn* conn);

extern int  pqUpdateVersionDate(PGconn* conn, char* version_id, struct tm* date);
extern int  pqUpdateVersionNote(PGconn* conn, char* version_id, char* note);

#endif
