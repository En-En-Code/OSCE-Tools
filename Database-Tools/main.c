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

#include <libpq-fe.h>
#include <git2.h>
#include <svn_cmdline.h>
#include "clihelpers.h"
#include "pqhelpers.h"

int main(int argc, char** argv) {
    const char* conninfo;
    PGconn*     conn;

    if (argc > 1) {
        conninfo = argv[1];
    } else {
        conninfo = "dbname=engine_db";
    }

    if (svn_cmdline_init("svn", stderr) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    conn = pqInitConnection(conninfo);
    git_libgit2_init();

    cliRootLoop(conn);
    
    PQfinish(conn);
    git_libgit2_shutdown();

    return EXIT_SUCCESS;
}
