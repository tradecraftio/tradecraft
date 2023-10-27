#!/usr/bin/env bash
#
# Copyright (c) 2019-2021 The Bitcoin Core developers
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

export CONTAINER_NAME=ci_native_nowallet
export DOCKER_NAME_TAG=ubuntu:18.04  # Use bionic to have one config run the tests in python3.6, see doc/dependencies.md
export PACKAGES="python3-zmq clang-7 llvm-7 libc++abi-7-dev libc++-7-dev"  # Use clang-7 to test C++17 compatibility, see doc/dependencies.md
export DEP_OPTS="NO_WALLET=1 CC=clang-7 CXX='clang++-7 -stdlib=libc++'"
export GOAL="install"
export FREICOIN_CONFIG="--enable-reduce-exports CC=clang-7 CXX='clang++-7 -stdlib=libc++'"
