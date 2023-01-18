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
    char* input = (char*)malloc(4096);
    input[0] = '\0';
    while (input[0] != 'Q') {
        cliListCommands();
        if (fgets(input, 4096, stdin) == NULL) {
            fprintf(stderr, "fgets returned a NULLPTR.\n");
            continue;
        }
        if (input[strlen(input) -1] == '\n')
            input[strlen(input) -1] = '\0';
        input[0] = toupper(input[0]);
        
        switch (input[0]) {
            case 'E':
                listAllEngines(conn);
                break;
            case 'V':
                char* engine_name = strchr(input, ' ');
                if (engine_name == NULL) {
                    fprintf(stderr, "Name of engine expected.\n");
                    break;
                }
                listAllVersions(conn, engine_name + 1);
                break;
            case 'N':
                char* new_engine_name = cliAllocEngineName();
                //char** author_names = cliAllocAuthors();
                //char** source_uri = cliAddNewSourceURIs();
                //cliAddNewVersion(conn);
                free(new_engine_name);
                break;
            case 'Q':
                // Intentional no error, since 'Q' quits loop.
                break;
            default:
                fprintf(stderr, "Command %c not expected.\n", input[0]);
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

// Memory is allocated by to store the engine_name.
// Free must be called after finishing using the returned value.
inline char* cliAllocEngineName() {
    char* engine_name = (char*)malloc(256);
    engine_name[0] = '\0';
    do {
        printf("Name of engine (cannot be empty): ");
	} while (fgets(engine_name, 256, stdin) == NULL ||
	        (engine_name[0] == '\0' || engine_name[0] == '\n'));
	
	if (engine_name[strlen(engine_name) -1] == '\n')
        engine_name[strlen(engine_name) -1] = '\0';
	
    return engine_name;
}
