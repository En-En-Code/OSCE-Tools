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
#include "globals.h"
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Memory is allocated by this function to store ptr.
// Free must be called when finished with the returned value.
void* errhandMalloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Not enough memory to perform allocation.\n");
        exit(1);
    }
    return ptr;
}

// Memory is allocated by this function to store ptr.
// Free must be called when finished with the returned value.
void* errhandCalloc(size_t num, size_t size) {
    void* ptr = calloc(num, size);
    if (ptr == NULL) {
        fprintf(stderr, "Not enough memory to perform allocation.\n");
        exit(1);
    }
    return ptr;
}

// Memory is allocated by this function to store new_ptr.
// Free must be called when finished with the returned value.
// Note this function can possibly free memory if size = 0.
void* errhandRealloc(void* ptr, size_t size) {
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
char* errhandStrdup(const char* s) {
    char* new_s = strdup(s);
    if (new_s == NULL) {
        fprintf(stderr, "not enough memory to perform allocation.\n");
        free(new_s);
        exit(1);
    }
    return new_s;
}

// Frees a version struct created with previous calls to malloc on each char*
// element
void freeVersion(version v) {
    freeRevision(v.rev);
    free(v.versionNum);
    free(v.programLang);
    free(v.license);
    free(v.note);
}

void freeCodeLink(code_link cl) {
    free(cl.id);
    free(cl.uri);
    free(cl.vcs);
}

revision* allocRevision(char* id, char* frag_type, char* frag_val,
                        int val_is_null) {
    revision* rev = (revision*)errhandMalloc(sizeof(revision));
    rev->code_id = errhandStrdup(id);
    rev->type = (strncmp(frag_type, "branch", 6) == 0)   ? 1
                : (strncmp(frag_type, "commit", 6) == 0) ? 2
                : (strncmp(frag_type, "revnum", 6) == 0) ? 4
                                                         : 8;
    if (val_is_null) {
        rev->val = NULL;
    } else {
        rev->val = errhandStrdup(frag_val);
    }
    return rev;
}

void freeRevision(revision r) {
    free(r.code_id);
    if (r.val != NULL) {
        free(r.val);
    }
}

// converts a char* of the format %Y-%m-%d into a time_t
time_t readDate(const char* date_str) {
    const char* date_str_ptr = date_str;

    struct tm stored_date = {0};
    stored_date.tm_year = atoi(date_str_ptr) - 1900;
    date_str_ptr += strcspn(date_str_ptr, "-") + 1;
    stored_date.tm_mon = atoi(date_str_ptr) - 1;
    date_str_ptr += strcspn(date_str_ptr, "-") + 1;
    stored_date.tm_mday = atoi(date_str_ptr) + 1;

    return mktime(&stored_date);
}

// A pair of helper functions based on https://stackoverflow.com/a/5467788
// Deletes a directory and its contents recursively.
int rm_file(const char* fpath, const struct stat* sb, int typeflag,
            struct FTW* ftwbuf) {
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int rm_file_recursive(char* path) {
    return nftw(path, rm_file, 64, FTW_DEPTH | FTW_PHYS);
}
