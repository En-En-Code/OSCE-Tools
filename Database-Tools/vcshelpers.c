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

#include "vcshelpers.h"
#include "globals.h"
#include "pqhelpers.h"
#include <git2.h>
#include <libpq-fe.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <svn_client.h>
#include <svn_config.h>
#include <svn_error.h>
#include <svn_opt.h>
#include <svn_pools.h>
#include <svn_props.h>
#include <time.h>
#include <unistd.h>

const int HASH_RECORD_LENGTH = 7;
const int MAX_PTHREADS = 8;

sem_t conn_lock;
sem_t idx_lock;
int   scan_idx;

// Returns the number of engines with updates found, or -1 on failure.
int vcsUpdateScan(PGconn* conn) {
    clock_t   start = clock();
    PGresult* res = pqAllocAllBranchRevisions(conn);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    if (pqCreateUpdateTable(conn) == -1) {
        PQclear(res);
        return -1;
    }

    sem_init(&conn_lock, 0, 1);
    scan_idx = 0;
    sem_init(&idx_lock, 0, 1);
    pthread_t         tid[MAX_PTHREADS];
    scan_thread_info* td = errhandCalloc(MAX_PTHREADS, sizeof(*td));
    for (int i = 0; i < MAX_PTHREADS; i++) {
        td[i].res = res;
        td[i].conn = conn;
        td[i].count = 0;
        pthread_create(&tid[i], NULL, vcsUpdateScanThread, &(td[i]));
    }

    int update_count = 0;
    for (int i = 0; i < MAX_PTHREADS; i++) {
        pthread_join(tid[i], NULL);
        update_count += td[i].count;
    }

    PQclear(res);
    printf("\n");
    pqSummarizeUpdateTable(conn);
    pqDropUpdateTable(conn);

    sem_destroy(&conn_lock);
    sem_destroy(&idx_lock);
    free(td);
    clock_t end = clock();
    double  len = (end - start) * 1000 / CLOCKS_PER_SEC;
    printf("\nCPU time: %.1f ms\n", len);

    return update_count;
}

// Helper function to vcsUpdateScan that runs concurrently
void* vcsUpdateScanThread(void* td) {
    scan_thread_info* thread_info = td;

    PGresult* res = thread_info->res;
    PGconn*   conn = thread_info->conn;
    int       tuples = PQntuples(res);
    int       update_count = 0;

    sem_wait(&idx_lock);
    int i = scan_idx;
    scan_idx += 1;
    sem_post(&idx_lock);
    while (i < tuples) {
        // Read vcs_name to decide what to do.
        char*     vcs_name = PQgetvalue(res, i, 4);
        revision* rev;
        if (strncmp(vcs_name, "git", 3) == 0) {
            rev = allocRevision(PQgetvalue(res, i, 6), PQgetvalue(res, i, 2),
                                PQgetvalue(res, i, 3), PQgetisnull(res, i, 3));
            time_t commit_time =
                vcsRevisionCommitTimeGit(rev, PQgetvalue(res, i, 1));
            update_count += vcsScanDateHelper(conn, res, i, commit_time);
            freeRevision(*rev);
            free(rev);
        } else if (strncmp(vcs_name, "svn", 3) == 0) {
            apr_pool_t* pool = svn_pool_create(NULL);
            rev = allocRevision(PQgetvalue(res, i, 6), PQgetvalue(res, i, 2),
                                PQgetvalue(res, i, 3), PQgetisnull(res, i, 3));
            time_t commit_time =
                vcsRevisionCommitTimeSvn(rev, PQgetvalue(res, i, 1), pool);
            update_count += vcsScanDateHelper(conn, res, i, commit_time);
            svn_pool_destroy(pool);
            freeRevision(*rev);
            free(rev);
        } else if (strncmp(vcs_name, "n/a", 3) == 0) {
            sem_wait(&conn_lock);
            pqInsertUpdate(conn, PQgetvalue(res, i, 0));
            sem_post(&conn_lock);
            fflush(stdout);
        } else if (strncmp(vcs_name, "rhv", 3) != 0) {
            // I choose (for the moment), to not be informed about engines
            // residing in archives.
            fprintf(stderr, "Unexpected vcs %s of %s\n", vcs_name,
                    PQgetvalue(res, i, 1));
            fflush(stderr);
        }
        sem_wait(&idx_lock);
        i = scan_idx;
        scan_idx += 1;
        sem_post(&idx_lock);
    }
    thread_info->count = update_count;
    return NULL; // I don't need anything returned really.
}

// A helper function to offload performing the date comparison in the update
// scan. Returns 1 if the date pulled is later than the date in the database, 0
// otherwise.
int vcsScanDateHelper(PGconn* conn, PGresult* res, int idx,
                      time_t commit_time) {
    int ret = 0;
    sem_wait(&conn_lock);
    char* stored_date_str =
        pqAllocLatestVersionDate(conn, PQgetvalue(res, idx, 5));
    sem_post(&conn_lock);
    time_t stored_time = readDate(stored_date_str);
    free(stored_date_str);

    if (commit_time > stored_time) {
        sem_wait(&conn_lock);
        pqInsertUpdate(conn, PQgetvalue(res, idx, 0));
        sem_post(&conn_lock);
        ret = 1;
        printf("!");
    } else {
        printf(".");
    }
    fflush(stdout);
    return ret;
}

// With the way engines having multiple sources works now, source cannot be
// NULL. Its allocation and deallocation are now handled by the cli.
int vcsUpdateRevisionInfo(PGconn* conn, char* version_id, code_link* source) {
    if (strncmp(source->vcs, "git", 3) == 0) {
        revision* rev = pqAllocRevisionFromVersion(conn, version_id);
        if (rev == NULL) {
            fprintf(
                stderr,
                "Revision info could not be obtained from the version info.\n");
            return -1;
        }
        git_commit* commit = vcsAllocRevisionCommitGit(rev, source->uri);
        freeRevision(*rev);
        free(rev);
        if (commit == NULL) {
            return -1;
        }

        time_t commit_time = git_commit_time(commit);

        const char*    commit_summary = git_commit_summary(commit);
        const git_oid* hash = git_commit_id(commit);
        char*          note =
            errhandMalloc(strlen(commit_summary) + HASH_RECORD_LENGTH + 4);
        sprintf(note, "[");
        // git_oid_nfmt could technically fail, but surely it won't :Clueless:
        git_oid_nfmt((note + 1), HASH_RECORD_LENGTH, hash);
        sprintf((note + 1 + HASH_RECORD_LENGTH), "] %s", commit_summary);

        pqUpdateVersionDate(conn, version_id, gmtime(&commit_time));
        pqUpdateVersionNote(conn, version_id, note);

        free(note);
        git_commit_free(commit);
    } else if (strncmp(source->vcs, "svn", 3) == 0) {
        apr_pool_t* pool = svn_pool_create(NULL);
        revision*   rev = pqAllocRevisionFromVersion(conn, version_id);
        if (rev == NULL) {
            fprintf(
                stderr,
                "Revision info could not be obtained from the version info.\n");
            svn_pool_destroy(pool);
            return -1;
        }
        svn_commit* commit = vcsAllocRevisionCommitSvn(rev, source->uri, pool);
        freeRevision(*rev);
        free(rev);
        if (commit == NULL) {
            svn_pool_destroy(pool);
            freeCodeLink(*source);
            free(source);
            return -1;
        }

        time_t commit_date = readDate(
            svn_prop_get_value(commit->revprops, SVN_PROP_REVISION_DATE));

        const char* commit_summary =
            svn_prop_get_value(commit->revprops, SVN_PROP_REVISION_LOG);
        if (commit_summary == NULL) {
            commit_summary = "";
        }
        char* note = errhandMalloc(strcspn(commit_summary, "\n") +
                                   floor(log10(commit->rev_num) + 1) + 5);
        sprintf(note, "[r%ld] %.*s", commit->rev_num,
                (int)strcspn(commit_summary, "\n"), commit_summary);

        pqUpdateVersionDate(conn, version_id, gmtime(&commit_date));
        pqUpdateVersionNote(conn, version_id, note);

        free(note);
        svn_pool_destroy(pool);
    } else {
        fprintf(stderr, "Sources not using a version control system cannot "
                        "automatically be updated.\n");
    }

    return 0;
}

// Returns time of last commit, or negative numbers for errors.
time_t vcsRevisionCommitTimeGit(revision* rev, char* uri) {
    git_commit* commit = vcsAllocRevisionCommitGit(rev, uri);
    if (commit == NULL) {
        return -1;
    }

    time_t update_time = git_commit_time(commit);
    git_commit_free(commit);

    return update_time;
}

time_t vcsRevisionCommitTimeSvn(revision* rev, char* uri, apr_pool_t* pool) {
    svn_commit* commit = vcsAllocRevisionCommitSvn(rev, uri, pool);
    if (commit == NULL) {
        return -1;
    }

    return readDate(
        svn_prop_get_value(commit->revprops, SVN_PROP_REVISION_DATE));
}

// Returns a pointer to the last git commit made to HEAD in uri. Must be freed.
git_commit* vcsAllocRevisionCommitGit(revision* rev, char* uri) {
    size_t size = 256;
    char*  path = errhandMalloc(size);
    while (getcwd(path, size - 12) == NULL) {
        size *= 2;
        path = errhandRealloc(path, size);
    }
    strcat(path, "/tempXXXXXX");
    path = mkdtemp(path);
    if (path == NULL) {
        fprintf(stderr, "Temporary filepath not created sucessfully.\n");
        return NULL;
    }

    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    git_repository*   repo = NULL;
    const char*       url = uri;
    char              frag_type = rev->type;
    if (frag_type == 4) {
        fprintf(stderr, "Revision number is not a valid identifier in Git.\n");
        return NULL;
    }

    clone_opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_NONE;
    // Shallow cloning!! :D
    // Use cutting-edge libgit2 (https://github.com/libgit2/libgit2/pull/6557)
    // This, on my then-current database of ~50 engines, reduced the CPU time of
    // vcsUpdateScan from 42.094 s to 11.276 s !
    // Caveat: This may not work for commit hash-based checks. Will test.
    clone_opts.fetch_opts.depth = 1;
    if (frag_type == 8) {
        clone_opts.fetch_opts.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_ALL;
    } else {
        clone_opts.fetch_opts.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_NONE;
    }

    int err = git_clone(&repo, url, path, &clone_opts);
    if (err < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "Error %d/%d: %s\n", err, e->klass, e->message);
        rm_file_recursive(path);
        free(path);
        return NULL;
    }

    git_oid oid;
    if (frag_type == 1) {
        if (rev->val) {
            char* template = errhandStrdup("refs/remotes/origin/");
            template = errhandRealloc(template, 21 + strlen(rev->val));
            strcat(template, rev->val);
            err = git_reference_name_to_id(&oid, repo, template);
            free(template);
        } else {
            err = git_reference_name_to_id(&oid, repo, "HEAD");
        }
    } else if (frag_type == 2) {
        err = git_oid_fromstr(&oid, rev->val);
    } else {
        char* template = errhandStrdup("refs/tags/");
        template = errhandRealloc(template, 11 + strlen(rev->val));
        strcat(template, rev->val);
        err = git_reference_name_to_id(&oid, repo, template);
        free(template);
    }
    if (err < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "Error %d/%d: %s", err, e->klass, e->message);
        git_repository_free(repo);
        rm_file_recursive(path);
        free(path);
        return NULL;
    }

    git_commit* commit = NULL;
    err = git_commit_lookup(&commit, repo, &oid);
    if (err < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "Error %d/%d: %s", err, e->klass, e->message);
        git_repository_free(repo);
        rm_file_recursive(path);
        free(path);
        return NULL;
    }

    git_repository_free(repo);
    rm_file_recursive(path);
    free(path);

    return commit;
}

// Memory is allocated by pool, which must be freed when finished.
svn_commit* vcsAllocRevisionCommitSvn(revision* rev, char* uri,
                                      apr_pool_t* pool) {
    svn_error_t* err;
    const char*  url = uri;

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

    svn_commit* commit = apr_palloc(pool, sizeof(svn_commit));
    // TODO: Set up revnum, tag, and branch-based pulls.
    commit->rev_num = SVN_INVALID_REVNUM;
    svn_opt_revision_t svn_rev;
    svn_rev.kind = svn_opt_revision_head;
    err = svn_client_revprop_list(&commit->revprops, url, &svn_rev,
                                  &commit->rev_num, ctx, pool);
    if (err != NULL) {
        svn_handle_error2(err, stderr, 0, "svn_err: ");
        return NULL;
    }

    return commit;
}
