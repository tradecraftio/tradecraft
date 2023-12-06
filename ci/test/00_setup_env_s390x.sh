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

export HOST=s390x-linux-gnu
export PACKAGES="python3-zmq"
export CONTAINER_NAME=ci_s390x
export CI_IMAGE_NAME_TAG="docker.io/s390x/debian:bookworm"
export TEST_RUNNER_EXTRA="--exclude feature_init,rpc_bind,feature_bind_extra"  # Excluded for now, see https://github.com/bitcoin/bitcoin/issues/17765#issuecomment-602068547
export RUN_FUNCTIONAL_TESTS=true
export GOAL="install"
export FREICOIN_CONFIG="--enable-reduce-exports"
