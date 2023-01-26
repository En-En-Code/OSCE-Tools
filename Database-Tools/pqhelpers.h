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

extern PGconn* pqInitConnection(const char* conninfo);
extern void pqPrintTable(PGresult* res);

extern void pqListAllEngines(PGconn* conn);
extern void pqListAllEnginesWithName(PGconn* conn, char* engine_name);
extern void pqListAllVersions(PGconn* conn, char* engine_name);

extern int  pqAddNewEngine(PGconn* conn, char* engine_name);
extern int  pqAddNewAuthor(PGconn* conn, int engine_id, char* authors);
extern int  pqAddNewSource(PGconn* conn, int engine_id, char* sources);
