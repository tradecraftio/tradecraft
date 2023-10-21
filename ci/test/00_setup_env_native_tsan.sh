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

export CONTAINER_NAME=ci_native_tsan
export CI_IMAGE_NAME_TAG=ubuntu:23.04  # Version 23.04 will reach EOL in Jan 2024, and can be replaced by "ubuntu:24.04" (or anything else that ships the wanted clang version).
export PACKAGES="clang-16 llvm-16 libclang-rt-16-dev libc++abi-16-dev libc++-16-dev python3-zmq"
export DEP_OPTS="CC=clang-16 CXX='clang++-16 -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER -DDEBUG_LOCKCONTENTION' CXXFLAGS='-g' --with-sanitizers=thread"
