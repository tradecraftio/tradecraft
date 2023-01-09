#!/usr/bin/env bash
#
# Copyright (c) 2020-2021 The Bitcoin Core developers
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

export HOST=i686-pc-linux-gnu
export CONTAINER_NAME=ci_i686_multiprocess
export DOCKER_NAME_TAG=ubuntu:20.04
export PACKAGES="cmake python3 python3-pip llvm clang g++-multilib"
export DEP_OPTS="DEBUG=1 MULTIPROCESS=1"
export GOAL="install"
export FREICOIN_CONFIG="--enable-debug CC='clang -m32' CXX='clang++ -m32' LDFLAGS='--rtlib=compiler-rt -lgcc_s'"
export TEST_RUNNER_ENV="FREICOIND=freicoin-node"
export TEST_RUNNER_EXTRA="--nosandbox"
export PIP_PACKAGES="lief"
