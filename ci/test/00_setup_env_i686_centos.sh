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
export CONTAINER_NAME=ci_i686_centos
export DOCKER_NAME_TAG=quay.io/centos/centos:stream8
export DOCKER_PACKAGES="gcc-c++ glibc-devel.x86_64 libstdc++-devel.x86_64 glibc-devel.i686 libstdc++-devel.i686 ccache libtool make git python3 python3-zmq which patch lbzip2 xz procps-ng dash rsync coreutils bison"
export GOAL="install"
export FREICOIN_CONFIG="--enable-zmq --with-gui=qt5 --enable-reduce-exports"
export CONFIG_SHELL="/bin/dash"
export TEST_RUNNER_ENV="LC_ALL=en_US.UTF-8"
