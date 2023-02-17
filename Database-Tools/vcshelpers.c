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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libpq-fe.h>
#include <git2.h>
#include "vcshelpers.h"
#include "pqhelpers.h"

// Returns the number of engines with updates found, or -1 on failure.
inline int vcsUpdateScan(PGconn* conn) {
    PGresult* res = pqAllocAllSources(conn);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    git_libgit2_init();
    
    int updateCount = 0;
    for (int i = 0; i < PQntuples(res); i += 1) {
        // Read vcs_name to decide what to do.
        if (strncmp(PQgetvalue(res, i, 3), "git", 3) == 0) {
            time_t commit_time = vcsUpdateScanGit(PQgetvalue(res, i, 2));
            char* stored_date_str = pqAllocLatestVersionDate(conn, PQgetvalue(res, i, 1));
            char* stored_date_ptr = stored_date_str;

            // stored_date_str is in %Y-%m-%d format.
            struct tm stored_date = { 0 };
            stored_date.tm_year = atoi(stored_date_ptr) - 1900;
            stored_date_ptr += strcspn(stored_date_ptr, "-") + 1;
            stored_date.tm_mon = atoi(stored_date_ptr) - 1;
            stored_date_ptr += strcspn(stored_date_ptr, "-") + 1;
            stored_date.tm_mday = atoi(stored_date_ptr);
            free(stored_date_str);

            time_t stored_time = mktime(&stored_date);

            if (commit_time > stored_time) {
                printf("%s has updates available at %s.\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 2));
                updateCount += 1;
            }
        } else if (strncmp(PQgetvalue(res, i, 3), "svn", 3) == 0) {
            // TODO
        } else if (strncmp(PQgetvalue(res, i, 3), "cvs", 3) == 0) {
            // TODO
        } else if (strncmp(PQgetvalue(res, i, 3), "n/a", 3) == 0) {
            printf("%s may have updates; check %s manually.\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 2));
        } else if (strncmp(PQgetvalue(res, i, 3), "rhv", 3) != 0) {
            // I choose (for the moment), to not be informed about engines residing in archives.
            fprintf(stderr, "Unrecognized vcs %s\n", PQgetvalue(res, i, 2));
        }
    }

    PQclear(res);
    git_libgit2_shutdown();

    return updateCount;
}

// Returns time of last commit, or negative numbers for errors.
inline time_t vcsUpdateScanGit(char* uri) {
    git_repository* repo = NULL;
    const char* url = uri;
    char path[4096];
    if (getcwd(path, 4090) == NULL) {
        fprintf(stderr, "Absolute filepath too long.\n");
        return -1;
    }
    strcat(path, "/temp");

    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    clone_opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_NONE;
    int err = git_clone(&repo, url, path, &clone_opts);
    if (err < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "Error %d/%d: %s\n", err, e->klass, e->message);
        return -1;
    }

    git_oid oid;
    err = git_reference_name_to_id(&oid, repo, "HEAD");
    if (err < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "Error %d/%d: %s", err, e->klass, e->message);
        return -1;
    }

    git_commit* latest_commit = NULL;
    err = git_commit_lookup(&latest_commit, repo, &oid);
    if (err < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "Error %d/%d: %s", err, e->klass, e->message);
        return -1;
    }

    time_t commit_time = git_commit_time(latest_commit);

    git_commit_free(latest_commit);
    git_repository_free(repo);
    rm_file_recursive(path);

    return commit_time;
}