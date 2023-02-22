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

#include <git2.h>

#ifndef VCS_H
#define VCS_H

extern int vcsUpdateScan(PGconn* conn);
extern int vcsUpdateTrunkInfo(PGconn* conn, char* version_id);

extern time_t vcsLastTrunkCommitTimeGit(char* uri);

extern git_commit* vcsAllocLastTrunkCommitGit(char* uri);

#endif VCS_H