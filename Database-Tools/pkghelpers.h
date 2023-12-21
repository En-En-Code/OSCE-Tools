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

#ifndef PKGHELPERS_H
#define PKGHELPERS_H

extern char*  pkgAllocStringFromFile();
extern size_t pkgStoreStringToFile(char* pkgbuild);
extern size_t pkgCreateDefaultFile(char* engine_name, char* version_name, char* note, char* uri,
                                    char* license, char* vcs_name, char* code_lang_name,
                                    char* frag_type, char* frag_val);
extern int    pkgRunMakepkg();

#endif
