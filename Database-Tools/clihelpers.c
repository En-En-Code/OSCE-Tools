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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> //for toupper
#include <string.h>
#include <libpq-fe.h>
#include "clihelpers.h"
#include "pqhelpers.h"

struct Version {
    char versionNum[256];
    unsigned int releaseDate[3]; //0 for year, 1 for month, 2 for day
    char programLang[256];
    char license[256];
    int xboard; //xboard and uci are bool-like
    int uci;
    char* notes;
};

inline void cliLoop(PGconn* conn) {
    char* input = (char*)cliMalloc(4096);
    input[0] = '\0';

    while (input[0] != 'Q') {
        cliListCommands();
        input[cliReadInput(input, 4096)] = '\0';
        input[0] = toupper(input[0]);
        
        switch (input[0]) {
            case 'E':
                listAllEngines(conn);
                break;
            case 'V':
                char* engine_name = strchr(input, ' ');
                if (engine_name == NULL) {
                    fprintf(stderr, "Name of engine expected.\n\n");
                    break;
                }
                listAllVersions(conn, engine_name + 1);
                break;
            case 'N':
                char* new_engine_name = cliAllocEngineName();
                char* author_names = cliAllocNDSeries("author", 256);
                char* sources = cliAllocNDSeries("URI", 4096);
                //cliAddNewVersion(conn);
                free(new_engine_name);
                free(author_names);
                free(sources);
                break;
            case 'Q':
                // Intentional no error, since 'Q' quits loop.
                break;
            default:
                fprintf(stderr, "Command %c not expected.\n\n", input[0]);
        }
    }
    free(input);
}

inline void cliListCommands() {
    printf("Accepted database commands:\n");
    printf("E (List all engines)\n");
    printf("V [NAME] (List all versions of all engines with name NAME)\n");
    //printf("N (Create new engine)\n");
    //printf("R (Create relation)\n");
    printf("Q (Quit)\n");
}

// An fgets-stdin wrapper which handles possible fgets errors.
// Returns the number of bytes before a newline read, which is far more useful than s.
inline int cliReadInput(char* s, int size) {
    if (fgets(s, size, stdin) == NULL) {
        fprintf(stderr, "fgets returned a NULLPTR.\n");
        exit(1);
    }
    return strcspn(s, "\r\n");
}

// Memory is allocated by this function to store ptr.
// Free must be called when finished with the returned value.
inline void* cliMalloc(size_t size) {
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
inline void* cliRealloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        fprintf(stderr, "Not enough memory to perform allocation.\n");
        exit(1);
    }
    return new_ptr;
}

// Memory is allocated by this function to store the engine_name.
// Free must be called when finished with the returned value.
inline char* cliAllocEngineName() {
    char* engine_name = (char*)cliMalloc(256);
    engine_name[0] = '\0';

    do {
        printf("Name of engine (cannot be empty): ");
        engine_name[cliReadInput(engine_name, 256)] = '\0';
	} while (engine_name[0] == '\0');
	
    return engine_name;
}

// Memory is allocated by this function to store author_names.
// Free must be called when finished with the returned value.
// The format is a string of values, each value separated by a newline.
inline char* cliAllocNDSeries(char* name, int size) {
    char* nd_strings = NULL;
    int total_bytes = 0;

    while (total_bytes == 0) {
        int bytes_written = size;

        while (bytes_written > 0) {
            nd_strings = (char*)cliRealloc(nd_strings, bytes_written);
            printf("Enter one %s (Leave empty to finish adding %ss): ", name, name);
            bytes_written = cliReadInput(nd_strings + total_bytes, size);
            total_bytes += bytes_written + 1; //jump over the newline/stray terminator
        }

        // This might seem strange to do, but since a stray null terminator could 
        // be inserted, I need a length which I know to be accurate.
        total_bytes = strlen(nd_strings);
        
        // Remove every newline from the end of the string
        while (total_bytes > 0) {
            if (nd_strings[total_bytes-1] != '\n') {
                break;
            }
            total_bytes -= 1;
            nd_strings[total_bytes] = '\0';
        }
        if (total_bytes == 0) {
            // Preparing to go again.
            free(nd_strings); // Free the memory, since we need to reallocate.
            nd_strings = NULL; // If names is not null, cliRealloc would double free.
            fprintf(stderr, "No %ss inserted. Minimum one %s needed.\n", name, name);
        }
    }
    
	return nd_strings;
}
