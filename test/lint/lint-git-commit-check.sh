#!/usr/bin/env bash
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
#
# Linter to check that commit messages have a new line before the body
# or no body at all

export LC_ALL=C

EXIT_CODE=0

while getopts "?" opt; do
  case $opt in
    ?)
      echo "Usage: $0 [N]"
      echo "       COMMIT_RANGE='<commit range>' $0"
      echo "       $0 -?"
      echo "Checks unmerged commits, the previous N commits, or a commit range."
      echo "COMMIT_RANGE='47ba2c3...ee50c9e' $0"
      exit ${EXIT_CODE}
    ;;
  esac
done

if [ -z "${COMMIT_RANGE}" ]; then
    if [ -n "$1" ]; then
      COMMIT_RANGE="HEAD~$1...HEAD"
    else
      # This assumes that the target branch of the pull request will be master.
      MERGE_BASE=$(git merge-base HEAD master)
      COMMIT_RANGE="$MERGE_BASE..HEAD"
    fi
fi

while IFS= read -r commit_hash  || [[ -n "$commit_hash" ]]; do
    n_line=0
    while IFS= read -r line || [[ -n "$line" ]]; do
        n_line=$((n_line+1))
        length=${#line}
        if [ $n_line -eq 2 ] && [ "$length" -ne 0 ]; then
            echo "The subject line of commit hash ${commit_hash} is followed by a non-empty line. Subject lines should always be followed by a blank line."
            EXIT_CODE=1
        fi
    done < <(git log --format=%B -n 1 "$commit_hash")
done < <(git log "${COMMIT_RANGE}" --format=%H)

exit ${EXIT_CODE}
