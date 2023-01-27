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

inline void cliLoop(PGconn* conn) {
    char* input = (char*)cliMalloc(4096);
    input[0] = '\0';
    char* engine_name;
    int engine_id;
    
    while (input[0] != 'Q') {
        cliListCommands();
        input[cliReadInput(input, 4096)] = '\0';
        input[0] = toupper(input[0]);
        
        switch (input[0]) {
            case 'E':
                pqListAllEngines(conn);
                break;
            case 'V':
                engine_name = strchr(input, ' ');
                if (engine_name == NULL) {
                    fprintf(stderr, "Name of engine expected.\n\n");
                    break;
                }
                pqListAllVersions(conn, engine_name + 1);
                break;
            case 'N':
                engine_name = cliAllocInputString("Name of engine", 256);
                char* author_names = cliAllocNDSeries("author", 256);
                //char* sources = cliAllocNDSeries("URI", 4096);
                //version version_info = cliCreateNewVersion();
                
                engine_id = 1; //pqAddNewEngine(conn, engine_name);
                if (engine_id != -1) {
                    int start_loc = 0;
                    int end_loc = strcspn((author_names + start_loc), "\n");
                    while (start_loc != end_loc) {
                        *(author_names + end_loc) = '\0';
                        printf("%s %d %d\n", (author_names + start_loc), start_loc, end_loc);
                        //pqAddNewAuthor(conn, engine_id, (author_names + start_loc));
                        start_loc = end_loc + 1;
                        end_loc = start_loc + strcspn((author_names + start_loc), "\n");
                    }
                    
                    //pqAddNewAuthor(conn, engine_id, author_names);
                    //pqAddNewSource(conn, engine_id, sources);
                }
                
                free(engine_name);
                free(author_names);
                //free(sources);
                //cliFreeVersion(version_info);
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
    return strcspn(s, "\n");
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
        free(ptr);
        exit(1);
    }
    return new_ptr;
}

// Memory is allocated by this function to store the input.
// Free must be called when finished with the returned value.
inline char* cliAllocInputString(char* explan, size_t size) {
    char* input = (char*)cliMalloc(size);
    input[0] = '\0';

    do {
        printf("%s (cannot be empty): ", explan);
        input[cliReadInput(input, size)] = '\0';
	} while (input[0] == '\0');
	
    return input;
}

// Memory is allocated by this function to store nd_strings.
// Free must be called when finished with the returned value.
// The format is a string of values, each value separated by a newline.
inline char* cliAllocNDSeries(char* name, size_t size) {
    char* nd_strings = NULL;
    int total_bytes = 0;

    while (total_bytes == 0) {
        int bytes_written = 0;

        do {
            nd_strings = (char*)cliRealloc(nd_strings, total_bytes + size);
            printf("Enter one %s (Leave empty to finish adding %ss): ", name, name);
            bytes_written = cliReadInput(nd_strings + total_bytes, size);
            total_bytes += bytes_written + 1; //jump over the newline/stray terminator
        } while (bytes_written > 0);

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

// Creates a new version struct.
// This function allocates memory several times, so it has a special free
// function, cliFreeVersion, to free everything when done with the struct.
inline version cliCreateNewVersion() {
    version version_data = {0};
    char* buff = (char*)cliMalloc(4096);
    
    version_data.versionNum = cliAllocInputString("Version identifier", 256);
    
    while (version_data.releaseDate[0] == 0) {
        printf("Year of release (number except 0): ");
        cliReadInput(buff, 16);
        version_data.releaseDate[0] = atoi(buff);
    }
    while (version_data.releaseDate[1] < 1 || version_data.releaseDate[1] > 12) {
        printf("Month of release (number between 1 and 12): ");
        cliReadInput(buff, 16);
        version_data.releaseDate[1] = atoi(buff);
    }
    // TODO: There is a validation method for this, I'm sure, but I'm a little lazy atm.
    while (version_data.releaseDate[2] < 1  || version_data.releaseDate[2] > 31) {
        printf("Day of release (number between 1 and 31): ");
        cliReadInput(buff, 16);
        version_data.releaseDate[2] = atoi(buff);
    }

    version_data.programLang = cliAllocInputString("Programming language", 64);
    version_data.license = cliAllocInputString("License", 64);

    printf("Other notes about this version: ");
    buff[cliReadInput(buff, 4096)] = '\0';
    version_data.notes = buff;

    return version_data;
}

inline void cliFreeVersion(version v) {
    free(v.versionNum);
    free(v.programLang);
    free(v.license);
    free(v.notes);
}
