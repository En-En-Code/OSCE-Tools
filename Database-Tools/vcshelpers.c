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
#include <math.h>
#include <libpq-fe.h>
#include <git2.h>
#include <svn_client.h>
#include <svn_config.h>
#include <svn_error.h>
#include <svn_opt.h>
#include <svn_pools.h>
#include <svn_props.h>
#include "vcshelpers.h"
#include "pqhelpers.h"
#include "globals.h"

const int HASH_RECORD_LENGTH = 7;

// Returns the number of engines with updates found, or -1 on failure.
inline int vcsUpdateScan(PGconn* conn) {
    PGresult* res = pqAllocAllSources(conn);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int updateCount = 0;
    for (int i = 0; i < PQntuples(res); i += 1) {
        // Read vcs_name to decide what to do.
        if (strncmp(PQgetvalue(res, i, 3), "git", 3) == 0) {
            time_t commit_time = vcsLastTrunkCommitTimeGit(PQgetvalue(res, i, 2));
            char* stored_date_str = pqAllocLatestVersionDate(conn, PQgetvalue(res, i, 1));
            time_t stored_time = readDate(stored_date_str);
            free(stored_date_str);

            if (commit_time > stored_time) {
                printf("\n%s has updates available at %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 2));
                fflush(stdout);
                updateCount += 1;
            } else {
                printf(".");
                fflush(stdout);
            }
        } else if (strncmp(PQgetvalue(res, i, 3), "svn", 3) == 0) {
            apr_pool_t* pool = svn_pool_create(NULL);
            time_t commit_time = vcsLastTrunkCommitTimeSvn(PQgetvalue(res, i, 2), pool);
            char* stored_date_str = pqAllocLatestVersionDate(conn, PQgetvalue(res, i, 1));
            time_t stored_time = readDate(stored_date_str);
            free(stored_date_str);

            if (commit_time > stored_time) {
                printf("\n%s has updates available at %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 2));
                fflush(stdout);
                updateCount += 1;
            } else {
                printf(".");
                fflush(stdout);
            }
            svn_pool_destroy(pool);
        } else if (strncmp(PQgetvalue(res, i, 3), "cvs", 3) == 0) {
            // TODO
        } else if (strncmp(PQgetvalue(res, i, 3), "n/a", 3) == 0) {
            printf("\n%s may have updates; check %s manually.\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 2));
            fflush(stdout);
        } else if (strncmp(PQgetvalue(res, i, 3), "rhv", 3) != 0) {
            // I choose (for the moment), to not be informed about engines residing in archives.
            fprintf(stderr, "Unrecognized vcs %s for %s\n", PQgetvalue(res, i, 2), PQgetvalue(res, i, 0));
            fflush(stderr);
        }
    }

    PQclear(res);

    return updateCount;
}

inline int vcsUpdateTrunkInfo(PGconn* conn, char* version_id) {
    code_link* source = pqAllocSourceFromVersion(conn, version_id);
    if (source == NULL) {
        return -1;
    }

    if (strncmp(source->vcs, "git", 3) == 0) {
        git_commit* last_commit = vcsAllocLastTrunkCommitGit(source->uri);
        if (last_commit == NULL) {
            freeCodeLink(*source);
            free(source);
            return -1;
        }

        time_t commit_time = git_commit_time(last_commit);

        const char* commit_summary = git_commit_summary(last_commit);
        const git_oid* hash = git_commit_id(last_commit);
        char* note = errhandMalloc(strlen(commit_summary) + HASH_RECORD_LENGTH + 4);
        sprintf(note, "[");
        // git_oid_nfmt could technically fail, but surely it won't :Clueless:
        git_oid_nfmt((note + 1), HASH_RECORD_LENGTH, hash);
        sprintf((note + 1 + HASH_RECORD_LENGTH), "] %s", commit_summary);

        pqUpdateVersionDate(conn, version_id, gmtime(&commit_time));
        pqUpdateVersionNote(conn, version_id, note);

        free(note);
        git_commit_free(last_commit);
    } else if (strncmp(source->vcs, "svn", 3) == 0) {
        apr_pool_t* pool = svn_pool_create(NULL);
        svn_commit* last_commit = vcsAllocLastTrunkCommitSvn(source->uri, pool);
        if (last_commit == NULL) {
            svn_pool_destroy(pool);
            freeCodeLink(*source);
            free(source);
            return -1;
        }

        time_t commit_date = readDate(svn_prop_get_value(last_commit->revprops, SVN_PROP_REVISION_DATE));

        const char* commit_summary = svn_prop_get_value(last_commit->revprops, SVN_PROP_REVISION_LOG);
        if (commit_summary == NULL) {
            commit_summary = "";
        }
        char* note = errhandMalloc(strcspn(commit_summary, "\n") + floor(log10(last_commit->rev_num) + 1) + 5);
        sprintf(note, "[r%ld] %.*s", last_commit->rev_num, (int)strcspn(commit_summary, "\n"), commit_summary);

        pqUpdateVersionDate(conn, version_id, gmtime(&commit_date));
        pqUpdateVersionNote(conn, version_id, note);
        
        free(note);
        svn_pool_destroy(pool);
    } else if (strncmp(source->vcs, "cvs", 3) == 0) {
        //TODO
    } else {
        fprintf(stderr, "Sources not using a version control system cannot automatically be updated.\n");
    }

    freeCodeLink(*source);
    free(source);
    return 0;
}

// Returns time of last commit, or negative numbers for errors.
inline time_t vcsLastTrunkCommitTimeGit(char* uri) {
    git_commit* last_commit = vcsAllocLastTrunkCommitGit(uri);
    if (last_commit == NULL) {
        return -1;
    }

    time_t last_update_time = git_commit_time(last_commit);
    git_commit_free(last_commit);

    return last_update_time;
}

inline time_t vcsLastTrunkCommitTimeSvn(char* uri, apr_pool_t* pool) {
    svn_commit* last_commit = vcsAllocLastTrunkCommitSvn(uri, pool);
    if (last_commit == NULL) {
        return -1;
    }

    return readDate(svn_prop_get_value(last_commit->revprops, SVN_PROP_REVISION_DATE));
}

// Returns a pointer to the last git commit made to HEAD in uri. Must be freed.
inline git_commit* vcsAllocLastTrunkCommitGit(char* uri) {
    git_repository* repo = NULL;
    const char* url = uri;
    char path[4096];
    if (getcwd(path, 4090) == NULL) {
        fprintf(stderr, "Absolute filepath too long.\n");
        return NULL;
    }
    strcat(path, "/temp");

    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    clone_opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_NONE;
    int err = git_clone(&repo, url, path, &clone_opts);
    if (err < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "Error %d/%d: %s\n", err, e->klass, e->message);
        return NULL;
    }

    git_oid oid;
    err = git_reference_name_to_id(&oid, repo, "HEAD");
    if (err < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "Error %d/%d: %s", err, e->klass, e->message);
        return NULL;
    }

    git_commit* last_commit = NULL;
    err = git_commit_lookup(&last_commit, repo, &oid);
    if (err < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "Error %d/%d: %s", err, e->klass, e->message);
        return NULL;
    }

    git_repository_free(repo);
    rm_file_recursive(path);

    return last_commit;
}

// Memory is allocated by pool, which must be freed when finished.
inline svn_commit* vcsAllocLastTrunkCommitSvn(char* uri, apr_pool_t* pool) {
    svn_error_t* err;
    const char* url = uri;

    err = svn_config_ensure(NULL, pool);
    if (err != NULL) {
        svn_handle_error2(err, stderr, 0, "svn_err: ");
        return NULL;
    }

    svn_client_ctx_t* ctx = NULL;
    err = svn_client_create_context2(&ctx, NULL, pool);
    if (err != NULL) {
        svn_handle_error2(err, stderr, 0, "svn_err: ");
        return NULL;
    }
    err = svn_config_get_config(&ctx->config, NULL, pool);
    if (err != NULL) {
        svn_handle_error2(err, stderr, 0, "svn_err: ");
        return NULL;
    }

    svn_commit* last_commit = apr_palloc(pool, sizeof(svn_commit));
    last_commit->rev_num = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_head;
    err = svn_client_revprop_list(&last_commit->revprops, url, &rev, &last_commit->rev_num, ctx, pool);
    if (err != NULL) {
        svn_handle_error2(err, stderr, 0, "svn_err: ");
        return NULL;
    }

    return last_commit;
}