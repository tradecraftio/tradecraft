#!/usr/bin/env bash
#
# Copyright (c) The Freicoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

export LC_ALL=C

# Fixes permission issues when there is a container UID/GID mismatch with the owner
# of the mounted freicoin src dir.
git config --global --add safe.directory /freicoin

export PATH="/python_build/bin:${PATH}"

if [ -z "$1" ]; then
  LOCAL_BRANCH=1 bash -ic "./ci/lint/06_script.sh"
else
  exec "$@"
fi
