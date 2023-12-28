#!/usr/bin/env sh
# Copyright 2023 En-En-Code
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if [ type makepkg &>/dev/null ]; then
	echo "makepkg utility not installed."
	exit 1
fi

# Create a temporary directory with a copy of PKGBUILD as a workspace.
mkdir -p build && cd build
cp ../PKGBUILD PKGBUILD

# Download and extract files and call prepare()
makepkg --nobuild
if [ $? != 0 ]; then
	echo "makepkg --nobuild failed to extract sources"
	exit 1
fi

# Using Perl regex, match everything after the '=' to obtain naming data.
_pkgname=$(grep -oP "^pkgname=\K[a-z0-9@._+-]+$" PKGBUILD)
if [ $? != 0 ]; then
	echo "grep pkgname failed to match"
	exit 2
fi
_pkgver=$(grep -oP "^pkgver=\K[A-Za-z0-9.]+$" PKGBUILD)
if [ $? != 0 ]; then
	echo "grep pkgver failed to match"
	exit 2
fi
_pkgrel=$(grep -oP "^pkgrel=\K[0-9]+$" PKGBUILD)
if [ $? != 0 ]; then
	echo "grep pkgrel failed to match"
	exit 2
fi
sourcetar="$_pkgname-$_pkgver-$_pkgrel.src.tar.gz"

# Archive then compress the source files
# find "src/$_pkgname" -path "src/$_pkgname/.git" -prune -o -print |
cd src
tar cfz "$sourcetar" --exclude-vcs --exclude-vcs-ignores "$_pkgname"
if [ $? != 0 ]; then
	echo "tar failed to archive files without modification"
	exit 3
fi
mv "$sourcetar" "../$sourcetar"
cd ..

# Build the package extracted by the previous step
makepkg --noextract
if [ $? != 0 ]; then
	echo "makepkg failed to build the extracted sources"
	exit 1
fi

# Clean up, moving the package, the source tarball, and
# the pkgbuild tarball to a dedicated directory
cp PKGBUILD ../PKGBUILD
cd ..
mkdir -p "Engines/$_pkgname" "Engines/share/$_pkgname"
mv "build/pkg/$_pkgname/usr/bin/$_pkgname" "Engines/$_pkgname/$_pkgname-$_pkgver"
mv "build/pkg/$_pkgname/usr/share/$_pkgname"/* "Engines/share/$_pkgname"
mv "build/$_pkgname-$_pkgver-$_pkgrel-x86_64.pkg.tar.zst" "Engines/$_pkgname/"
mv "build/$sourcetar" "Engines/$_pkgname/"
rm -rf build
