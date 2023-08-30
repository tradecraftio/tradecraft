#!/usr/bin/env bash
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
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

export CONTAINER_NAME=ci_native_nowallet_libfreicoinkernel
export CI_IMAGE_NAME_TAG=ubuntu:focal
# Use minimum supported python3.7 (or python3.8, as best-effort) and clang-8, see doc/dependencies.md
export PACKAGES="python3-zmq clang-8 llvm-8 libc++abi-8-dev libc++-8-dev"
export DEP_OPTS="NO_WALLET=1 CC=clang-8 CXX='clang++-8 -stdlib=libc++'"
export GOAL="install"
export NO_WERROR=1
export FREICOIN_CONFIG="--enable-reduce-exports CC=clang-8 CXX='clang++-8 -stdlib=libc++' --enable-experimental-util-chainstate --with-experimental-kernel-lib --enable-shared"
