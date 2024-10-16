#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
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

export LC_ALL=C

set -ex

if [ -n "$CIRRUS_PR" ]; then
  COMMIT_RANGE="HEAD~..HEAD"
  if [ "$(git rev-list -1 HEAD)" != "$(git rev-list -1 --merges HEAD)" ]; then
    echo "Error: The top commit must be a merge commit, usually the remote 'pull/${PR_NUMBER}/merge' branch."
    false
  fi
else
  # Otherwise, assume that a merge commit exists. This merge commit is assumed
  # to be the base, after which linting will be done. If the merge commit is
  # HEAD, the range will be empty.
  COMMIT_RANGE="$( git rev-list --max-count=1 --merges HEAD )..HEAD"
fi
export COMMIT_RANGE

echo
git log --no-merges --oneline "$COMMIT_RANGE"
echo
test/lint/commit-script-check.sh "$COMMIT_RANGE"
RUST_BACKTRACE=1 "${LINT_RUNNER_PATH}/test_runner"

if [ "$CIRRUS_REPO_FULL_NAME" = "freicoin/freicoin" ] && [ "$CIRRUS_PR" = "" ] ; then
    # Sanity check only the last few commits to get notified of missing sigs,
    # missing keys, or expired keys. Usually there is only one new merge commit
    # per push on the master branch and a few commits on release branches, so
    # sanity checking only a few (10) commits seems sufficient and cheap.
    git log HEAD~10 -1 --format='%H' > ./contrib/verify-commits/trusted-sha512-root-commit
    git log HEAD~10 -1 --format='%H' > ./contrib/verify-commits/trusted-git-root
    mapfile -t KEYS < contrib/verify-commits/trusted-keys
    git config user.email "ci@ci.ci"
    git config user.name "ci"
    ${CI_RETRY_EXE} gpg --keyserver hkps://keys.openpgp.org --recv-keys "${KEYS[@]}" &&
    ./contrib/verify-commits/verify-commits.py;
fi
