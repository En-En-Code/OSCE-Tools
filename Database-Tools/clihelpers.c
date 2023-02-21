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
#include "vcshelpers.h"
#include "globals.h"

inline void cliRootLoop(PGconn* conn) {
    char* input = (char*)errhandMalloc(4096);
    input[0] = '\0';
    char* engine_name = NULL;
    int engine_id;
    
    while (input[0] != 'Q') {
        cliListRootCommands();
        cliReadInput(input, 4096);
        input[0] = toupper(input[0]);
        
        switch (input[0]) {
            case 'E':
                pqListEngines(conn);
                break;
            case 'N':
                engine_name = cliAllocInputString("Name of engine", 256);
                printf("Note(s) for every version of %s: ", engine_name);
                cliReadInput(input, 4096);
                char* engine_id_str = pqAddNewEngine(conn, engine_name, strlen(input)?input:NULL);
                if (engine_id_str != NULL) {
                    cliEngineLoop(conn, engine_name, engine_id_str);
                }
                free(engine_name);
                free(engine_id_str);
                break;
            case 'S':
                engine_name = strchr(input, ' ');
                if (engine_name == NULL) {
                    fprintf(stderr, "Name of engine exepcted.\n");
                    break;
                }
                engine_name += 1; // Move to the index after the space.
                engine_id = cliObtainEngineIdFromName(conn, engine_name);
                if (engine_id != -1) {
                    char engine_id_str[25];
                    snprintf(engine_id_str, 25, "%d", engine_id);
                    cliEngineLoop(conn, engine_name, engine_id_str);
                }
                break;
            case 'U':
                vcsUpdateScan(conn);
                break;
            case 'Q':
                // Intentional no error, since 'Q' quits loop.
                break;
            default:
                fprintf(stderr, "Command %c not expected.\n", input[0]);
        }
        printf("\n");
    }
    
    free(input);
}

inline void cliEngineLoop(PGconn* conn, char* engine_name, char* engine_id) {
    char* input = (char*)errhandMalloc(4096);
    input[0] = '\0';
    char* parent_engine_name = NULL;
    int parent_engine_id;

    while (input[0] != 'X') {
        cliListEngineCommands(engine_name);
        cliReadInput(input, 4096);
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
            case 'C':
                code_link source = cliAllocCodeLink();
                pqAddNewSource(conn, engine_id, source);
                freeCodeLink(source);
                break;
            case 'V':
                version version_info = cliAllocVersion();
                pqAddNewVersion(conn, engine_id, version_info);
                freeVersion(version_info);
                break;
            case 'I':
                parent_engine_name = strchr(input, ' ');
                if (parent_engine_name == NULL) {
                    fprintf(stderr, "Name of engine exepcted.\n");
                    break;
                }
                parent_engine_name += 1; // Move to the index after the space.
                parent_engine_id = cliObtainEngineIdFromName(conn, parent_engine_name);
                if (parent_engine_id != -1) {
                    pqAddNewInspiration(conn, engine_id, parent_engine_id);
                }
                break;
            case 'D':
                parent_engine_name = strchr(input, ' ');
                if (parent_engine_name == NULL) {
                    fprintf(stderr, "Name of engine expected.\n");
                    break;
                }
                parent_engine_name += 1; // Move to the index after the space.
                parent_engine_id = cliObtainEngineIdFromName(conn, parent_engine_name);
                if (parent_engine_id != -1) {
                    pqAddNewPredecessor(conn, engine_id, parent_engine_id);
                }
                break;
            case 'S':
                char* version_name = strchr(input, ' ');
                if (version_name == NULL) {
                    fprintf(stderr, "Name of version expected.\n");
                    break;
                }
                version_name += 1; // Move to the index after the space.
                char* version_id = cliObtainVersionIdFromName(conn, engine_id, version_name);
                if (version_id != NULL) {
                    cliVersionLoop(conn, engine_name, version_id, version_name);
                    free(version_id);
                }
                break;
            case 'X':
                //Intentional no error, since 'X' quits loop.
                break;
            default:
                fprintf(stderr, "Command %c not expected.\n", input[0]);
        }
        printf("\n");
    }
    free(input);
}

inline void cliVersionLoop(PGconn* conn, char* engine_name, char* version_id, char* version_name) {
    char* input = (char*)errhandMalloc(4096);
    input[0] = '\0';

    while (input[0] != 'X') {
        cliListVersionCommands(engine_name, version_name);
        cliReadInput(input, 4096);
        input[0] = toupper(input[0]);
        switch (input[0]) {
            case 'O':
                char* os_name = strchr(input, ' ');
                if (os_name == NULL) {
                    fprintf(stderr, "Name of operating system expected.\n");
                    break;
                }
                os_name += 1;
                pqAddNewVersionOs(conn, version_id, os_name);
                break;
            case 'T':
                char* egtb_name = strchr(input, ' ');
                if (egtb_name == NULL) {
                    fprintf(stderr, "Name of endgame tablebase expected.\n");
                    break;
                }
                egtb_name += 1;
                pqAddNewVersionEgtb(conn, version_id, egtb_name);
                break;
            case 'X':
                //Intentional no error, since 'X' quits loop.
                break;
            default:
                fprintf(stderr, "Command %c not expected.\n", input[0]);
        }
        printf("\n");
    }
    free(input);
}

inline void cliListRootCommands() {
    printf("Accepted database commands:\n");
    printf("E (List all engines)\n");
    printf("N (Create new engine)\n");
    printf("S [NAME] (Select existing engine [NAME])\n");
    printf("U (Check engines for updates)\n");
    printf("Q (Quit)\n");
}

inline void cliListEngineCommands(char* engine_name) {
    printf("What would you like to do with %s?\n", engine_name);
    printf("P (Print info for %s)\n", engine_name);
    printf("A (Add new authors to %s)\n", engine_name);
    printf("C (Add new source code URI to %s)\n", engine_name);
    printf("V (Add new version of %s)\n", engine_name);
    printf("I [NAME] (Add engine [NAME] as an inspiration)\n");
    printf("D [NAME] (Add engine [NAME] as a predecessor)\n");
    printf("S [VER] (Select existing version [VER])\n");
    printf("X (Exit to the root menu)\n");
}

inline void cliListVersionCommands(char* engine_name, char* engine_version) {
    printf("What would you like to do with %s %s?\n", engine_name, engine_version);
    printf("O [OS] (Add operating system [OS] compatible with %s %s)\n", engine_name, engine_version);
    printf("T [EGTB] (Add endgame tablebase [EGTB] compatible with %s %s)\n", engine_name, engine_version);
    printf("X (Exit to the engine menu)\n");
}

// An fgets-stdin wrapper which handles possible fgets errors.
// Returns the number of bytes before a newline read, which is far more useful than s.
inline size_t cliReadInput(char* s, int size) {
    if (fgets(s, size, stdin) == NULL) {
        fprintf(stderr, "fgets returned a NULLPTR.\n");
        exit(1);
    }
    size_t length = strlen(s);
    if (length > 0) {
        if (s[length-1] != '\n') {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        } else {
            s[length-1] = '\0';
            length -= 1;
        }
    }
    return length;
}

// Memory is allocated by this function to store the input.
// Free must be called when finished with the returned value.
inline char* cliAllocInputString(char* explan, size_t size) {
    char* input = (char*)errhandMalloc(size);
    input[0] = '\0';

    do {
        printf("%s (cannot be empty): ", explan);
        cliReadInput(input, size);
    } while (input[0] == '\0');
    
    return input;
}

// Memory is allocated by this function to store nd_strings.
// Free must be called when finished with the returned value.
// The format is a string of values, each value separated by a newline.
inline char* cliAllocNDSeries(char* name, size_t size) {
    char* nd_strings = NULL;
    size_t total_bytes = 0;

    while (total_bytes == 0) {
        size_t bytes_written = 0;

        do {
            nd_strings = (char*)errhandRealloc(nd_strings, total_bytes + size);
            printf("Enter one %s (Leave empty to finish adding %ss): ", name, name);
            bytes_written = cliReadInput(nd_strings + total_bytes, size);
            nd_strings[total_bytes + bytes_written] = '\n';
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
inline int cliObtainEngineIdFromName(PGconn* conn, char* engine_name) {
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
            cliReadInput(input, 4096);
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

inline char* cliObtainVersionIdFromName(PGconn* conn, char* engine_id, char* version_name) {
    char* version_id = pqAllocVersionIdWithName(conn, engine_id, version_name);
    if (version_id == NULL) {
        fprintf(stderr, "No version %s associated with the engine.\n", version_name);
        return NULL;
    }
    return version_id;
}

// Creates a new code_link struct.
// This function allocates memory 2 times, so it has a special free
// function, freeCodeLink, to fre everything when done with the struct.
inline code_link cliAllocCodeLink() {
    code_link codeLink = {0};
    codeLink.uri = cliAllocInputString("Source URI", 4096);
    codeLink.vcs = cliAllocInputString("3-letter version control system abbrievation", 4);
    return codeLink;
}

// Creates a new version struct.
// This function allocates memory several times, so it has a special free
// function, freeVersion, to free everything when done with the struct.
inline version cliAllocVersion() {
    version version_data = {0};
    char* buff = (char*)errhandMalloc(4096);
    
    version_data.versionNum = cliAllocInputString("Version identifier", 256);
    
    struct tm release_date = { .tm_year = -1, .tm_mon = -1, .tm_mday = -1 };
    while (release_date.tm_year <= 0) {
        printf("Year of release (number greater than 1900): ");
        cliReadInput(buff, 16);
        release_date.tm_year = atoi(buff) - 1900;
    }
    int month_len[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((release_date.tm_year % 4 == 0 && release_date.tm_year % 100 != 0) ||
        release_date.tm_year % 400 == 0) {
        month_len[1] = 29;
    }
    while (release_date.tm_mon < 0 || release_date.tm_mon > 11) {
        printf("Month of release (number between 1 and 12): ");
        cliReadInput(buff, 16);
        release_date.tm_mon = atoi(buff) - 1;
    }
    
    while (release_date.tm_mday < 1 || release_date.tm_mday > month_len[release_date.tm_mon]) {
        printf("Day of release (number between 1 and %d): ", month_len[release_date.tm_mon]);
        cliReadInput(buff, 16);
        release_date.tm_mday = atoi(buff);
    }
    version_data.releaseDate = release_date;

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
    cliReadInput(buff, 4096);
    version_data.note = buff;

    return version_data;
}
