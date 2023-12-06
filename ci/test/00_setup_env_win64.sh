#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Copyright (c) 2010-2023 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of version 3 of the GNU Affero General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
# details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_win64
export CI_IMAGE_NAME_TAG="docker.io/amd64/ubuntu:22.04"  # Check that Jammy can cross-compile to win64
export HOST=x86_64-w64-mingw32
export DPKG_ADD_ARCH="i386"
export PACKAGES="nsis g++-mingw-w64-x86-64-posix wine-binfmt wine64 wine32 file"
export RUN_FUNCTIONAL_TESTS=false
export GOAL="deploy"
# Prior to 11.0.0, the mingw-w64 headers were missing noreturn attributes, causing warnings when
# cross-compiling for Windows. https://sourceforge.net/p/mingw-w64/bugs/306/
# https://github.com/mingw-w64/mingw-w64/commit/1690994f515910a31b9fb7c7bd3a52d4ba987abe
export BITCOIN_CONFIG="--enable-reduce-exports --enable-external-signer --disable-gui-tests CXXFLAGS=-Wno-return-type"
