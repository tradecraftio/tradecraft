#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Copyright (c) 2010-2022 The Freicoin Developers
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
export DOCKER_NAME_TAG=ubuntu:18.04  # Check that bionic can cross-compile to win64 (bionic is used in the gitian build as well)
export HOST=x86_64-w64-mingw32
export PACKAGES="python3 nsis g++-mingw-w64-x86-64 wine-binfmt wine64 file"
export RUN_FUNCTIONAL_TESTS=false
export RUN_SECURITY_TESTS="true"
export GOAL="deploy"
export FREICOIN_CONFIG="--enable-reduce-exports --disable-gui-tests --without-boost-process"

# Compiler for MinGW-w64 causes false -Wreturn-type warning.
# See https://sourceforge.net/p/mingw-w64/bugs/306/
export NO_WERROR=1
