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

#ifndef CLIHELPERS_H
#define CLIHELPERS_H

#include "globals.h"

extern void cliRootLoop(PGconn* conn);
extern void cliEngineLoop(PGconn* conn, char* engine_name, char* engine_id);
extern void cliVersionLoop(PGconn* conn, char* engine_name, char* version_id, char* version_name);
extern void cliListRootCommands();
extern void cliListEngineCommands(char* engine_name);
extern void cliListVersionCommands(char* engine_name, char* version_name);

extern size_t   cliReadInput(char* s, int size);
extern char*    cliAllocInputString(char* explan, size_t size);
extern char*    cliAllocNDSeries(char* name, size_t size);
extern int      cliObtainEngineIdFromName(PGconn* conn, char* engine_name);
extern char*    cliObtainVersionIdFromName(PGconn* conn, char* engine_id, char* version_name);

extern code_link    cliAllocCodeLink();
extern version      cliAllocVersion();

#endif