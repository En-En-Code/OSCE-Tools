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

typedef struct {
    char* versionNum;
    unsigned int releaseDate[3]; //0 for year, 1 for month, 2 for day
    char* programLang;
    char* license;
    char protocol; //A bit mask. 1 is xboard compat, 2 is uci copmat.
    char* notes;
} version;

extern void cliLoop(PGconn* conn);
extern void	cliListCommands();
extern int  cliReadInput(char* s, int size);

extern void*    cliMalloc(size_t size);
extern void*    cliRealloc(void* ptr, size_t size);

extern char*    cliAllocInputString(char* explan, int size);
extern char*    cliAllocNDSeries(char* name, int size);
extern version  cliCreateNewVersion();
extern void     cliFreeVersion(version v);
