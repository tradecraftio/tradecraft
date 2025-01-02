#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Copyright (c) 2010-2024 The Freicoin Developers
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
export CI_IMAGE_NAME_TAG="docker.io/ubuntu:22.04"
# Use minimum supported python3.9 (or best-effort 3.10) and clang-14, see doc/dependencies.md
export PACKAGES="python3-zmq clang-14 llvm-14 libc++abi-14-dev libc++-14-dev"
export DEP_OPTS="NO_WALLET=1 CC=clang-14 CXX='clang++-14 -stdlib=libc++'"
export GOAL="install"
export FREICOIN_CONFIG="--enable-reduce-exports --enable-experimental-util-chainstate --with-experimental-kernel-lib --enable-shared"
