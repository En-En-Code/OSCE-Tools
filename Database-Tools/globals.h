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

#include <time.h>

typedef struct {
    char* code_id;
    char  type; // 1 for branch, 2 for commit, 4 for revnum, 8 for tag.
    char* val;
} revision;

typedef struct {
    char*     versionNum;
    revision  rev;
    struct tm releaseDate;
    char*     programLang;
    char*     license;
    char      protocol; // A bit mask. 1 is xboard compat, 2 is uci copmat.
    char*     note;
} version;

typedef struct {
    char* id;
    char* uri;
    char* vcs;
} code_link;

extern void* errhandMalloc(size_t size);
extern void* errhandCalloc(size_t num, size_t size);
extern void* errhandRealloc(void* ptr, size_t size);
extern char* errhandStrdup(const char* s);

extern void freeVersion(version v);
extern void freeCodeLink(code_link cl);

extern revision* allocRevision(char* code_id, char* frag_type, char* frag_val,
                               int is_val_null);
extern void      freeRevision(revision r);

extern time_t readDate(const char* date_str);

struct FTW;
struct stat;
extern int rm_file(const char* fpath, const struct stat* sb, int typeflag,
                   struct FTW* ftwbuf);
extern int rm_file_recursive(char* fpath);

#endif
