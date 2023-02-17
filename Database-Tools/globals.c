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

#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include "globals.h"

// Memory is allocated by this function to store ptr.
// Free must be called when finished with the returned value.
inline void* errhandMalloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Not enough memory to perform allocation.\n");
        exit(1);
    }
    return ptr;
}

// Memory is allocated by this function to store new_ptr.
// Free must be called when finished with the returned value.
// Note this function can possibly free memory if size = 0.
inline void* errhandRealloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        fprintf(stderr, "Not enough memory to perform allocation.\n");
        free(ptr);
        exit(1);
    }
    return new_ptr;
}

// Memory is allocated by this function to store new_s
// Free must be called when finished with the returned value.
inline char* errhandStrdup(const char* s) {
    char* new_s = strdup(s);
    if (new_s == NULL) {
        fprintf(stderr, "not enough memory to perform allocation.\n");
        free(new_s);
        exit(1);
    }
    return new_s;
}

// Frees a version struct created with previous calls to malloc on each char* element
inline void freeVersion(version v) {
    free(v.versionNum);
    free(v.programLang);
    free(v.license);
    free(v.note);
}

inline void freeCodeLink(code_link cl) {
    free(cl.uri);
    free(cl.vcs);
}

// A pair of helper functions based on https://stackoverflow.com/a/5467788
// Deletes a directory and its contents recursively.
inline int rm_file(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

inline int rm_file_recursive(char* path) {
    return nftw(path, rm_file, 64, FTW_DEPTH | FTW_PHYS);
}