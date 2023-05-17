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

#ifndef VCSHELPERS_H
#define VCSHELPERS_H

#include <git2.h>
#include <apr_hash.h>
#include <svn_types.h>

// Why revprops couldn't just contain the revision number is beyond me
typedef struct {
    apr_hash_t* revprops;
    svn_revnum_t rev_num;
} svn_commit;

typedef struct {
    int thread_num;
    PGresult* res;
    PGconn* conn;
    int count;
} scan_thread_info;

extern int      vcsUpdateScan(PGconn* conn);
extern void*    vcsUpdateScanThread(void* td);
extern int      vcsUpdateTrunkInfo(PGconn* conn, char* version_id);

extern time_t vcsLastTrunkCommitTimeGit(char* uri);
extern time_t vcsLastTrunkCommitTimeSvn(char* uri, apr_pool_t* pool);

extern git_commit*  vcsAllocLastTrunkCommitGit(char* uri);
extern svn_commit*  vcsAllocLastTrunkCommitSvn(char* uri, apr_pool_t* pool);

#endif
