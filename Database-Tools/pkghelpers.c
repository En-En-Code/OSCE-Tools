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

#include "pkghelpers.h"
#include "globals.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    char* pkgbuild = (char*)errhandMalloc(len + 1);
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

size_t pkgCreateDefaultFile(char* engine_name, char* version_name, char* note,
                            char* uri, char* license, char* vcs_name,
                            char* code_lang_name, char* frag_type,
                            char* frag_val) {
    // In the common order as specified by
    // https://wiki.archlinux.org/title/PKGBUILD
    char* pkg_fmt = "# Maintainer: Your Name <your-name@example.org>\n"
                    "pkgname=%s\n"
                    "pkgver=%s\n"
                    "pkgrel=1\n"
                    "epoch=0\n"
                    "pkgdesc=\"%s\"\n"
                    "arch=('x86_64')\n"
                    "url=\"%s\"\n"
                    "license=(%s)\n"
                    "depends=(%s)\n"
                    "makedepends=(%s)\n"
                    "checkdepends=()\n"
                    "source=(\"%s\")\n"
                    "md5sums=('SKIP')\n\n"
                    "prepare() {\n  :\n}\n\n"
                    "pkgver() {\n  echo \"$pkgver\"\n}\n\n"
                    "build() {\n  :\n}\n\n"
                    "check() {\n  :\n}\n\n"
                    "package() {\n  :\n}";
    char* name_fmt = "%s-ce%s%s";
    char* source_fmt = "$pkgname::%s+$url%s%s";
    char* frag_fmt = "#%s=%s";

    // Replace all letters with the lowercase equivalent.
    // Replace all characters besides alphanum, @, ., _, +, and - with _.
    for (int i = 0; i < strlen(engine_name); i++) {
        if (isalnum(engine_name[i])) {
            engine_name[i] = tolower(engine_name[i]);
        } else if (engine_name[i] != '@' && engine_name[i] != '.' &&
                   engine_name[i] != '+' && engine_name[i] != '-') {
            engine_name[i] = '_';
        }
    }

    char* name_buf;
    int   name_len = 0;
    // If the version is based on a branch head, then label it with the vcs.
    if (!strcmp(frag_type, "branch")) {
        name_len = snprintf(NULL, 0, name_fmt, engine_name, "-", vcs_name);
        name_buf = errhandMalloc(name_len + 1);
        snprintf(name_buf, name_len + 1, name_fmt, engine_name, "-", vcs_name);
    } else {
        name_len = snprintf(NULL, 0, name_fmt, engine_name, "", "");
        name_buf = errhandMalloc(name_len + 1);
        snprintf(name_buf, name_len + 1, name_fmt, engine_name, "", "");
    }

    char* frag_buf;
    if (frag_val[0] != '\0') {
        int frag_len = snprintf(NULL, 0, frag_fmt, frag_type, frag_val);
        frag_buf = errhandMalloc(frag_len + 1);
        snprintf(frag_buf, frag_len + 1, frag_fmt, frag_type, frag_val);
    } else {
        frag_buf = errhandMalloc(1);
        frag_buf[0] = '\0';
    }

    // PKGBUILD is making a transition to SPDX identifiers, see
    // (https://rfc.archlinux.page/0016-spdx-license-identifiers/).
    // This simplifies my life a lot actually.
    char* license_buf;
    if (!strncmp(license, "Custom", 6)) {
        int license_len = snprintf(NULL, 0, "'custom:%s-license'", engine_name);
        license_buf = errhandMalloc(license_len + 1);
        snprintf(license_buf, license_len + 1, "'custom:%s-license'",
                 engine_name);
    } else if (!strncmp(license, "None", 4)) {
        license_buf = errhandStrdup("");
    } else {
        int license_len = snprintf(NULL, 0, "'%s'", license);
        license_buf = errhandMalloc(license_len + 1);
        snprintf(license_buf, license_len + 1, "'%s'", license);
    }

    char* dep_buf;
    char* makedep_buf;
    // Handles any dependencies based on the programming language.
    // TODO: The rest of the programming languages.
    if (!strcmp(code_lang_name, "C") || !strcmp(code_lang_name, "C++")) {
        dep_buf = errhandStrdup("'glibc'");
        makedep_buf = errhandStrdup("");
    } else if (!strncmp(code_lang_name, "Rust", 4)) {
        dep_buf = errhandStrdup("");
        makedep_buf = errhandStrdup("'cargo'");
    } else {
        dep_buf = errhandStrdup("");
        makedep_buf = errhandStrdup("");
    }

    char* source_buf;
    // Handles dependencies based on the version control system.
    if (!strncmp(vcs_name, "git", 3)) {
        int source_len =
            snprintf(NULL, 0, source_fmt, vcs_name, ".git", frag_buf);
        source_buf = errhandMalloc(source_len + 1);
        snprintf(source_buf, source_len + 1, source_fmt, vcs_name, ".git",
                 frag_buf);

        if (strlen(makedep_buf)) {
            makedep_buf = errhandRealloc(makedep_buf, 2 + strlen(makedep_buf));
            strcat(makedep_buf, " ");
        }
        makedep_buf = errhandRealloc(makedep_buf, 6 + strlen(makedep_buf));
        makedep_buf = strcat(makedep_buf, "'git'");
    } else if (!strncmp(vcs_name, "svn", 3)) {
        int source_len = snprintf(NULL, 0, source_fmt, vcs_name, "", frag_buf);
        source_buf = errhandMalloc(source_len + 1);
        snprintf(source_buf, source_len + 1, source_fmt, vcs_name, "",
                 frag_buf);

        if (strlen(makedep_buf)) {
            makedep_buf = errhandRealloc(makedep_buf, 2 + strlen(makedep_buf));
            strcat(makedep_buf, " ");
        }
        makedep_buf = errhandRealloc(makedep_buf, 13 + strlen(makedep_buf));
        makedep_buf = strcat(makedep_buf, "'subversion'");
    } else {
        source_buf = errhandStrdup(uri);
    }

    int pkg_len = snprintf(NULL, 0, pkg_fmt, name_buf, version_name, note, uri,
                           license_buf, dep_buf, makedep_buf, source_buf);
    char* pkg_buf = errhandMalloc(pkg_len + 1);
    snprintf(pkg_buf, pkg_len + 1, pkg_fmt, name_buf, version_name, note, uri,
             license_buf, dep_buf, makedep_buf, source_buf);

    size_t ret = pkgStoreStringToFile(pkg_buf);
    free(name_buf);
    free(license_buf);
    free(dep_buf);
    free(makedep_buf);
    free(source_buf);
    free(frag_buf);
    free(pkg_buf);

    return ret;
}

int pkgRunMakepkg() { return system("./pkg-run-makepkg.sh"); }
