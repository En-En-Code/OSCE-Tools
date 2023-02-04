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
#include "globals.h"

inline void cliRootLoop(PGconn* conn) {
    char* input = (char*)errhandMalloc(4096);
    input[0] = '\0';
    char* engine_name;
    int engine_id;
    
    while (input[0] != 'Q') {
        cliListRootCommands();
        input[cliReadInput(input, 4096)] = '\0';
        input[0] = toupper(input[0]);
        
        switch (input[0]) {
            case 'E':
                pqListEngines(conn);
                break;
            case 'N':
                engine_name = cliAllocInputString("Name of engine", 256);
                printf("Note(s) for every version of %s: ", engine_name);
                input[cliReadInput(input, 4096)] = '\0';
                engine_id = pqAddNewEngine(conn, engine_name, strlen(input)?input:NULL);
                //version version_info = cliCreateNewVersion();
                
                if (engine_id != -1) {
                    cliEngineLoop(conn, engine_name, engine_id);
                    //pqAddNewVersion(conn, engine_id, version_info);
                }
                
                free(engine_name);
                //cliFreeVersion(version_info);
                break;
            case 'S':
                engine_name = strchr(input, ' ');
                if (engine_name == NULL) {
                    fprintf(stderr, "Name of engine exepcted.\n\n");
                    break;
                }
                engine_name += 1; // Move to the index after the space.
                engine_id = cliObtainIdFromName(conn, engine_name);
                if (engine_id != -1) {
                    cliEngineLoop(conn, engine_name, engine_id);
                }
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

inline void cliEngineLoop(PGconn* conn, char* engine_name, int engine_id) {
    char* input = (char*)errhandMalloc(4096);
    input[0] = '\0';
    
    while (input[0] != 'X') {
        cliListEngineCommands(engine_name);
        input[cliReadInput(input, 4096)] = '\0';
        input[0] = toupper(input[0]);
        
        switch (input[0]) {
            case 'P':
                pqListAuthors(conn, engine_id);
                pqListSources(conn, engine_id);
                pqListVersions(conn, engine_id);
                break;
            case 'A':
                char* author_names = cliAllocNDSeries("author", 256);
                pqAddNewNDAuthors(conn, engine_id, author_names);
                free(author_names);
                break;
            case 'S':
                char* sources = cliAllocNDSeries("URI", 4096);
                pqAddNewNDSources(conn, engine_id, sources);
                free(sources);
                break;
            case 'X':
                //Intentional no error, since 'X' quits loop.
                printf("\n");
                break;
            default:
                fprintf(stderr, "Command %c not expected.\n\n", input[0]);
        }
    }
    free(input);
}

inline void cliListRootCommands() {
    printf("Accepted database commands:\n");
    printf("E (List all engines)\n");
    printf("N (Create new engine)\n");
    printf("S [NAME] (Select existing engine)\n");
    printf("Q (Quit)\n");
}

inline void cliListEngineCommands(char* engine_name) {
    printf("What would you like to do with %s?\n", engine_name);
    printf("P (Print info for %s)\n", engine_name);
    printf("A (Add new authors to %s)\n", engine_name);
    printf("S (Add new source code links to %s)\n", engine_name);
    printf("X (Exit to the root menu)\n");
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

// Memory is allocated by this function to store the input.
// Free must be called when finished with the returned value.
inline char* cliAllocInputString(char* explan, size_t size) {
    char* input = (char*)errhandMalloc(size);
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
            nd_strings = (char*)errhandRealloc(nd_strings, total_bytes + size);
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
            nd_strings = NULL; // If names is not null, errhandRealloc would double free.
            fprintf(stderr, "No %ss inserted. Minimum one %s needed.\n", name, name);
        }
    }
    
	return nd_strings;
}

// A helper function, which determines the ID of an engine based on its name.
// If multiple engines with the same name exist, asks for user to disambiguate.
// Returns -1 if no engine with the name exists.
inline int cliObtainIdFromName(PGconn* conn, char* engine_name) {
    int engine_id = -1;
    int* engine_id_list = pqAllocEngineIdsWithName(conn, engine_name);
    if (engine_id_list == NULL || *engine_id_list == 0) {
        fprintf(stderr, "No engines found with name %s.\n\n", engine_name);
        return -1;
    }
    if (*engine_id_list == 1) {
    // Exactly one engine was found with that name, so use the found ID.
        engine_id = *(engine_id_list + 1);
    } else {
        // Multiple engines were found with that name, so disambiguate them.
        char found_id = 0;
        char* input = (char*)errhandMalloc(4096);
        input[0] = '\0';
        while (!found_id) {
            printf("Multiple engines of the name %s found.\n", engine_name);
            pqListEnginesWithName(conn, engine_name);
            printf("Select an engine ID from the list to disambiguate: ");
            input[cliReadInput(input, 4096)] = '\0';
            engine_id = atoi(input);
            for (int i = 0; i < *engine_id_list; i += 1) {
                if (engine_id == *(engine_id_list + 1 + i)) {
                    found_id = 1;
                    break;
                }
            }
            if (found_id == 0) {
                fprintf(stderr, "ID %d not found in the ID list.\n", engine_id);
            }
        }
        free(input);
    }
    printf("\n");
    free(engine_id_list);
    return engine_id;
}

// Creates a new version struct.
// This function allocates memory several times, so it has a special free
// function, cliFreeVersion, to free everything when done with the struct.
inline version cliCreateNewVersion() {
    version version_data = {0};
    char* buff = (char*)errhandMalloc(4096);
    
    version_data.versionNum = cliAllocInputString("Version identifier", 256);
    
    while (version_data.releaseDate[0] <= 0) {
        printf("Year of release (number greater than 0): ");
        cliReadInput(buff, 16);
        version_data.releaseDate[0] = atoi(buff);
    }
    int month_len[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((version_data.releaseDate[0] % 4 == 0 && version_data.releaseDate[0] % 100 != 0) ||
        version_data.releaseDate[0] % 400 == 0) {
        month_len[1] = 29;
    }
    while (version_data.releaseDate[1] < 1 || version_data.releaseDate[1] > 12) {
        printf("Month of release (number between 1 and 12): ");
        cliReadInput(buff, 16);
        version_data.releaseDate[1] = atoi(buff);
    }
    
    while (version_data.releaseDate[2] < 1 || version_data.releaseDate[2] > month_len[version_data.releaseDate[1] -1]) {
        printf("Day of release (number between 1 and %d): ", month_len[version_data.releaseDate[1] -1]);
        cliReadInput(buff, 16);
        version_data.releaseDate[2] = atoi(buff);
    }

    version_data.programLang = cliAllocInputString("Programming language", 64);
    
    const char* protocols[2] = { "Xboard", "UCI" };
    for (int i = 0; i <= 1; i++) {
        while (1) {
            printf("Does engine support %s protocol (Y/N, T/F)? ", protocols[i]);
            cliReadInput(buff, 16);
            buff[0] = toupper(buff[0]);
            if (buff[0] == 'Y' || buff[0] == 'T') {
                version_data.protocol |= (1 << i);
                break;
            }
            if (buff[0] == 'N' || buff[0] == 'F') {
                break;
            }
        }
    }
    
    version_data.license = cliAllocInputString("License", 64);

    printf("Other notes about this version: ");
    buff[cliReadInput(buff, 4096)] = '\0';
    version_data.note = buff;

    return version_data;
}

inline void cliFreeVersion(version v) {
    free(v.versionNum);
    free(v.programLang);
    free(v.license);
    free(v.note);
}
