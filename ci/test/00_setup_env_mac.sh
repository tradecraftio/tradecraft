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

export CONTAINER_NAME=ci_macos_cross
export CI_IMAGE_NAME_TAG=ubuntu:20.04  # Check that Focal can cross-compile to macos
export HOST=x86_64-apple-darwin
export PACKAGES="cmake libz-dev libtinfo5 python3-setuptools xorriso"
export XCODE_VERSION=12.2
export XCODE_BUILD_ID=12B45b
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export GOAL="deploy"
export BITCOIN_CONFIG="--with-gui --enable-reduce-exports"
