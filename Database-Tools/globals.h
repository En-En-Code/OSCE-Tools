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

#ifndef GLOBALS_H
#define GLOBALS_H

typedef struct {
    char* versionNum;
    unsigned int releaseDate[3]; //0 for year, 1 for month, 2 for day
    char* programLang;
    char* license;
    char protocol; //A bit mask. 1 is xboard compat, 2 is uci copmat.
    char* notes;
} version;

#endif