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
#include <string.h>
#include "pkghelpers.h"
#include "globals.h"

char* pkgAllocStringFromFile() {
    FILE* fp = fopen("PKGBUILD", "rb");
    if (!fp) {
        fprintf(stderr, "Opening of PKGBUILD file failed.\n");
        return NULL;
    }
    long len;
    fseek(fp, 0L, SEEK_END);
    len = ftell(fp);
    rewind(fp);
    char* pkgbuild = (char*)errhandMalloc(len+1);
    if (fread(pkgbuild, sizeof(char), len, fp) != len) {
        fprintf(stderr, "Read from PKGBUILD failed.\n");
        free(pkgbuild);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    pkgbuild[len] = '\0';

    return pkgbuild;
}

size_t pkgStoreStringToFile(char* pkgbuild) {
    FILE* fp = fopen("PKGBUILD", "wb");
    if (!fp) {
        fprintf(stderr, "Creation or opening of PKGBUILD file failed.");
        return 0;
    }
    size_t bytes = fwrite(pkgbuild, sizeof(char), strlen(pkgbuild), fp);

    fclose(fp);

    return bytes;
}

size_t pkgCreateDefaultFile(char* engine_name, char* version_name, char* note, char* uri,
                            char* license, char* vcs_name, char* frag_type, char* frag_val) {
    char* pkg_fmt = "# Maintainer: You <You@example.org>\n"
                    "pkgname=%s\n"
                    "pkgver=%s\n"
                    "pkgrel=1\n"
                    "epoch=0\n"
                    "pkgdesc=%s\n"
                    "url='%s'\n"
                    "license=('%s')\n"
                    "source=('%s')\n"
                    "cksums('SKIP')\n"
                    "arch=('any')\n"
                    "depends=()\n"
                    "makedepends=(%s)\n"
                    "checkdepends=()\n\n"
                    "pkgver() {\n"
                    "}\n\n"
                    "prepare() {\n"
                    "}\n\n"
                    "build() {\n"
                    "}\n\n"
                    "check() {\n"
                    "}\n";
    char* source_fmt = "$pkgname::%s+$url%s%s";
    char* frag_fmt = "#%s=%s";

    char* frag_buf;
    if (frag_val[0] != '\0') {
        int frag_len = snprintf(NULL, 0, frag_fmt, frag_type, frag_val);
        frag_buf = errhandMalloc(frag_len + 1);
        snprintf(frag_buf, frag_len + 1, frag_fmt, frag_type, frag_val);
    } else {
        frag_buf = errhandMalloc(1);
        frag_buf[0] = '\0';
    }

    char* source_buf;
    char* vcs_dep;
    if (!strncmp(vcs_name, "git", 3)) {
        int source_len = snprintf(NULL, 0, source_fmt, vcs_name, ".git", frag_buf);
        source_buf = errhandMalloc(source_len + 1);
        snprintf(source_buf, source_len + 1, source_fmt, vcs_name, ".git", frag_buf);
        vcs_dep = "git";
    } else if (!strncmp(vcs_name, "svn", 3)) {
        int source_len = snprintf(NULL, 0, source_fmt, vcs_name, "", frag_buf);
        source_buf = errhandMalloc(source_len + 1);
        snprintf(source_buf, source_len + 1, source_fmt, vcs_name, "", frag_buf);
        vcs_dep = "subversion";
    } else {
        source_buf = errhandStrdup(uri);
        vcs_dep = "";
    }

    int pkg_len = snprintf(NULL, 0, pkg_fmt, engine_name, version_name, note, uri,
                            license, source_buf, vcs_dep);
    char* pkg_buf = errhandMalloc(pkg_len + 1);
    snprintf(pkg_buf, pkg_len + 1, pkg_fmt, engine_name, version_name, note, uri,
                            license, source_buf, vcs_dep);

    size_t ret = pkgStoreStringToFile(pkg_buf);
    free(source_buf);
    free(frag_buf);
    free(pkg_buf);
    return ret;
}
