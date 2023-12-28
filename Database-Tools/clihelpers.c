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

#include "clihelpers.h"
#include "globals.h"
#include "pkghelpers.h"
#include "pqhelpers.h"
#include "vcshelpers.h"
#include <ctype.h> //for toupper
#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cliRootLoop(PGconn* conn) {
    char* input = (char*)errhandMalloc(4096);
    input[0] = '\0';
    char* engine_name = NULL;
    int   engine_id;

    printf("Welcome to the database-cli!\n");
    while (input[0] != 'Q') {
        cliListRootCommands();
        input = cliReadLine(input);
        input[0] = toupper(input[0]);

        switch (input[0]) {
            case 'E':
                pqListEngines(conn);
                break;
            case 'N':
                input = cliRequestValue("Name of engine", input);
                engine_name = errhandStrdup(input);
                printf("Note(s) for every version of %s: ", engine_name);
                input = cliReadLine(input);
                char* engine_id_str = pqInsertEngine(
                    conn, engine_name, strlen(input) ? input : NULL);
                if (engine_id_str != NULL) {
                    cliEngineLoop(conn, engine_name, engine_id_str);
                    free(engine_id_str);
                }
                free(engine_name);
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
    }

    free(input);
}

void cliEngineLoop(PGconn* conn, char* engine_name, char* engine_id) {
    char* input = (char*)errhandMalloc(4096);
    input[0] = '\0';
    char* parent_engine_name = NULL;
    int   parent_engine_id;
    char* version_id = NULL;

    while (input[0] != 'X') {
        cliListEngineCommands(engine_name);
        input = cliReadLine(input);
        input[0] = toupper(input[0]);

        switch (input[0]) {
            case 'P':
                pqListNote(conn, engine_id);
                pqListAuthors(conn, engine_id);
                pqListSources(conn, engine_id);
                pqListVersions(conn, engine_id);
                break;
            case 'A':
                input = cliRequestValue("Author", input);
                char* author_name = errhandStrdup(input);
                pqInsertAuthor(conn, engine_id, author_name);
                free(author_name);
                break;
            case 'C': {
                code_link source = cliAllocCodeLink();
                pqInsertSource(conn, engine_id, source);
                freeCodeLink(source);
                break;
            }
            case 'N': {
                version version_info = cliAllocVersion(conn, engine_id);
                if (version_info.versionNum == NULL) {
                    fprintf(stderr,
                            "Version was not allocated successfully.\n");
                    break;
                }
                version_id = pqInsertVersion(conn, engine_id, version_info);
                if (version_id != NULL) {
                    cliVersionLoop(conn, engine_id, engine_name, version_id,
                                   version_info.versionNum);
                    free(version_id);
                }
                freeVersion(version_info);
                break;
            }
            case 'I':
                parent_engine_name = strchr(input, ' ');
                if (parent_engine_name == NULL) {
                    fprintf(stderr, "Name of engine expected.\n");
                    break;
                }
                parent_engine_name += 1; // Move to the index after the space.
                parent_engine_id =
                    cliObtainEngineIdFromName(conn, parent_engine_name);
                if (parent_engine_id != -1) {
                    pqInsertInspiration(conn, engine_id, parent_engine_id);
                }
                break;
            case 'D':
                parent_engine_name = strchr(input, ' ');
                if (parent_engine_name == NULL) {
                    fprintf(stderr, "Name of engine expected.\n");
                    break;
                }
                parent_engine_name += 1; // Move to the index after the space.
                parent_engine_id =
                    cliObtainEngineIdFromName(conn, parent_engine_name);
                if (parent_engine_id != -1) {
                    pqInsertPredecessor(conn, engine_id, parent_engine_id);
                }
                break;
            case 'S': {
                char* version_name = strchr(input, ' ');
                if (version_name == NULL) {
                    fprintf(stderr, "Name of version expected.\n");
                    break;
                }
                version_name += 1; // Move to the index after the space.
                version_id =
                    cliObtainVersionIdFromName(conn, engine_id, version_name);
                if (version_id != NULL) {
                    cliVersionLoop(conn, engine_id, engine_name, version_id,
                                   version_name);
                    free(version_id);
                }
                break;
            }
            case 'X':
                // Intentional no error, since 'X' quits loop.
                break;
            default:
                fprintf(stderr, "Command %c not expected.\n", input[0]);
        }
    }
    free(input);
}

void cliVersionLoop(PGconn* conn, char* engine_id, char* engine_name,
                    char* version_id, char* version_name) {
    char* input = (char*)errhandMalloc(4096);
    input[0] = '\0';

    while (input[0] != 'X') {
        cliListVersionCommands(engine_name, version_name);
        input = cliReadLine(input);
        input[0] = toupper(input[0]);
        switch (input[0]) {
            case 'P':
                pqListVersionDetails(conn, version_id);
                break;
            case 'O': {
                char* os_name = strchr(input, ' ');
                if (os_name == NULL) {
                    fprintf(stderr, "Name of operating system expected.\n");
                    break;
                }
                os_name += 1;
                pqInsertVersionOs(conn, version_id, os_name);
                break;
            }
            case 'T': {
                char* egtb_name = strchr(input, ' ');
                if (egtb_name == NULL) {
                    fprintf(stderr, "Name of endgame tablebase expected.\n");
                    break;
                }
                egtb_name += 1;
                pqInsertVersionEgtb(conn, version_id, egtb_name);
                break;
            }
            case 'U': {
                code_link* source = pqAllocSourceFromVersion(conn, version_id);
                if (source) {
                    vcsUpdateRevisionInfo(conn, version_id, source);
                    freeCodeLink(*source);
                    free(source);
                }
                break;
            }
            case 'W':
                pqExtractPkgbuild(conn, version_id);
                break;
            case 'M':
                if (pkgRunMakepkg()) {
                    fprintf(stderr, "Makepkg script returned an error.");
                }
                break;
            case 'S':
                pqUpdatePkgbuild(conn, version_id);
                break;
            case 'X':
                // Intentional no error, since 'X' quits loop.
                break;
            default:
                fprintf(stderr, "Command %c not expected.\n", input[0]);
        }
    }
    free(input);
}

void cliListRootCommands() {
    printf("\nAccepted database commands:\n");
    printf("E        (List all engines)\n");
    printf("N        (Create new engine)\n");
    printf("S [NAME] (Select existing engine [NAME])\n");
    printf("U        (Check engines for updates)\n");
    printf("Q        (Quit)\n");
}

void cliListEngineCommands(char* engine_name) {
    printf("\nWhat would you like to do with %s?\n", engine_name);
    printf("P        (Print info for %s)\n", engine_name);
    printf("A        (Add new author to %s)\n", engine_name);
    printf("C        (Add new source code URI to %s)\n", engine_name);
    printf("N        (Create new version of %s)\n", engine_name);
    printf("I [NAME] (Add engine [NAME] as an inspiration)\n");
    printf("D [NAME] (Add engine [NAME] as a predecessor)\n");
    printf("S [VER]  (Select existing version [VER])\n");
    printf("X        (Exit to the root menu)\n");
}

void cliListVersionCommands(char* engine_name, char* engine_version) {
    printf("\nWhat would you like to do with %s %s?\n", engine_name,
           engine_version);
    printf("P        (Print info for %s %s)\n", engine_name, engine_version);
    printf("O [OS]   (Add operating system [OS] compatible with %s %s)\n",
           engine_name, engine_version);
    printf("T [EGTB] (Add endgame tablebase [EGTB] compatible with %s %s)\n",
           engine_name, engine_version);
    printf("U        (Pull updates from HEAD)\n");
    printf("W        (Write PKGBUILD to current location)\n");
    printf("M        (Run makepkg to build engine using current PKGBUILD)\n");
    printf("S        (Store PKGBUILD in directory to %s %s)\n", engine_name,
           engine_version);
    printf("X        (Exit to the engine menu)\n");
}

// Overhauled client input reader. Might make more allocations, though of
// more consistent and reasonable sizes, with expansion if necessary.
char* cliReadLine(char* s) {
    int  temp_len = 256;
    int  buff_len = 0;
    int  used_len = 0;
    char temp[temp_len];
    s[0] = '\0';
    do {
        buff_len += temp_len;
        if (fgets(temp, temp_len, stdin) == NULL) {
            fprintf(stderr, "fgets returned a NULLPTR.\n");
            exit(1);
        }
        s = (char*)errhandRealloc(s, buff_len);
        strcat(s, temp);
        used_len = strlen(s);
    } while (s[used_len - 1] != '\n');
    s[used_len - 1] = '\0';

    return s;
}

// A wrapper for a very common use of cliReadLine, obtaining a particular
// data point to be entered into the database.
char* cliRequestValue(char* explan, char* s) {
    do {
        printf("%s (cannot be empty): ", explan);
        s = cliReadLine(s);
    } while (s[0] == '\0');
    return s;
}

// A helper function, which determines the ID of an engine based on its name.
// If multiple engines with the same name exist, asks for user to disambiguate.
// Returns -1 if no engine with the name exists.
int cliObtainEngineIdFromName(PGconn* conn, char* engine_name) {
    int  engine_id = -1;
    int* engine_id_list = pqAllocEngineIdsWithName(conn, engine_name);
    if (engine_id_list == NULL || engine_id_list[0] == 0) {
        fprintf(stderr, "No engines found with name %s.\n", engine_name);
        free(engine_id_list);
        return -1;
    }
    if (engine_id_list[0] == 1) {
        // Exactly one engine was found with that name, so use the found ID.
        engine_id = engine_id_list[1];
    } else {
        // Multiple engines were found with that name, so disambiguate them.
        char  found_id = 0;
        char* input = (char*)errhandMalloc(4096);
        input[0] = '\0';
        while (!found_id) {
            printf("Multiple engines of the name %s found.\n", engine_name);
            pqListEnginesWithName(conn, engine_name);
            printf("Select an engine ID from the list to disambiguate: ");
            input = cliReadLine(input);
            engine_id = atoi(input);
            for (int i = 0; i < engine_id_list[0]; i += 1) {
                if (engine_id == engine_id_list[i + 1]) {
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
    free(engine_id_list);
    return engine_id;
}

char* cliObtainVersionIdFromName(PGconn* conn, char* engine_id,
                                 char* version_name) {
    char* version_id = pqAllocVersionIdWithName(conn, engine_id, version_name);
    if (version_id == NULL) {
        fprintf(stderr, "No version %s associated with the engine.\n",
                version_name);
        return NULL;
    }
    return version_id;
}

// Creates a new code_link struct.
// This function allocates memory 2 times, so it has a special free
// function, freeCodeLink, to free everything when done with the struct.
code_link cliAllocCodeLink() {
    code_link codeLink = {0};
    char*     uri_str = errhandMalloc(256);
    char*     vcs_str = errhandMalloc(256);
    codeLink.uri = cliRequestValue("Source URI", uri_str);
    codeLink.vcs = cliRequestValue(
        "3-letter version control system abbrievation", vcs_str);
    return codeLink;
}

// Creates a new version struct.
// This function allocates memory several times, so it has a special free
// function, freeVersion, to free everything when done with the struct.
version cliAllocVersion(PGconn* conn, char* engine_id) {
    version version_data = {0};
    char*   buff = (char*)errhandMalloc(4096);

    size_t      dest_elems = 0;
    code_link** sources =
        pqAllocSourcesFromEngine(conn, engine_id, &dest_elems);
    int choice = -1;
    if (dest_elems < 1) {
        // If the engine has no sources, creating a version is not allowed
        fprintf(stderr,
                "Creating a version cannot be done without a source.\n");
        free(buff);
        // sources does not need to be freed because it was never mallocked
        return version_data;
    } else if (dest_elems == 1) {
        printf("One source, %s, found. Proceeding with this source.\n",
               sources[0]->uri);
        choice = 1;
    } else {
        // Select a source between the available ones for this engine.
        printf("Multiple sources to use found.\n");
        for (int i = 1; i <= dest_elems; i++) {
            printf("Opt. %d: %s\n", i, sources[i - 1]->uri);
        }
        while (choice <= 0 || choice > dest_elems) {
            printf("Select a source using its option number: ");
            buff = cliReadLine(buff);
            choice = atoi(buff);
        }
    }

    revision rev = {0};
    rev.code_id = errhandStrdup(sources[choice - 1]->id);
    do {
        printf(
            "Select the identifier type of branch, commit, revision number, or "
            "tag (B/C/R/T): ");
        buff = cliReadLine(buff);
        buff[0] = toupper(buff[0]);
    } while (buff[0] != 'B' && buff[0] != 'C' && buff[0] != 'R' &&
             buff[0] != 'T');

    if (buff[0] == 'B') {
        printf("Name of branch to watch (if blank, defaults to the repo's "
               "trunk): ");
        buff = cliReadLine(buff);
        rev.type = 1;
        if (buff[0] != '\0') {
            rev.val = errhandStrdup(buff);
        } else {
            rev.val = NULL;
        }
    } else if (buff[0] == 'C') {
        int hash_len;
        do {
            hash_len = 0;
            printf("Commit hash (40 hexadecimal characters): ");
            buff = cliReadLine(buff);
            while (isxdigit(buff[hash_len])) {
                hash_len += 1;
            }
        } while (hash_len < 40);
        rev.type = 2;
        rev.val = errhandStrdup(buff);
    } else if (buff[0] == 'R') {
        do {
            printf("Revision number: ");
            buff = cliReadLine(buff);
        } while (atoi(buff) <= 0);
        rev.type = 4;
        rev.val = errhandStrdup(buff);
    } else {
        buff = cliRequestValue("Name of tag", buff);
        rev.type = 8;
        rev.val = errhandStrdup(buff);
    }
    version_data.rev = rev;

    buff = cliRequestValue("Version identifier", buff);
    version_data.versionNum = errhandStrdup(buff);

    struct tm release_date = {.tm_year = -1, .tm_mon = -1, .tm_mday = -1};
    while (release_date.tm_year <= 0) {
        printf("Year of release (number greater than 1900): ");
        buff = cliReadLine(buff);
        release_date.tm_year = atoi(buff) - 1900;
    }
    int month_len[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((release_date.tm_year % 4 == 0 && release_date.tm_year % 100 != 0) ||
        release_date.tm_year % 400 == 0) {
        month_len[1] = 29;
    }
    while (release_date.tm_mon < 0 || release_date.tm_mon > 11) {
        printf("Month of release (number between 1 and 12): ");
        buff = cliReadLine(buff);
        release_date.tm_mon = atoi(buff) - 1;
    }

    while (release_date.tm_mday < 1 ||
           release_date.tm_mday > month_len[release_date.tm_mon]) {
        printf("Day of release (number between 1 and %d): ",
               month_len[release_date.tm_mon]);
        buff = cliReadLine(buff);
        release_date.tm_mday = atoi(buff);
    }
    version_data.releaseDate = release_date;

    buff = cliRequestValue("Programming language", buff);
    version_data.programLang = errhandStrdup(buff);

    const char* protocols[2] = {"Xboard", "UCI"};
    for (int i = 0; i <= 1; i++) {
        while (1) {
            printf("Does engine support %s protocol (Y/N, T/F)? ",
                   protocols[i]);
            buff = cliReadLine(buff);
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

    buff = cliRequestValue(
        "License (SPDX identifier, 'None', 'All-Rights-Reserved', or 'Custom')",
        buff);
    version_data.license = errhandStrdup(buff);

    printf("Other notes about this version: ");
    buff = cliReadLine(buff);
    version_data.note = errhandStrdup(buff);

    free(buff);
    for (int i = 0; i < dest_elems; i++) {
        freeCodeLink(*sources[i]);
        free(sources[i]);
    }
    free(sources);
    return version_data;
}
