#!/usr/bin/env python3
#
# Copyright (c) 2020-2022 The Bitcoin Core developers
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
#
# Linter to check that commit messages have a new line before the body
# or no body at all

import argparse
import os
import sys

from subprocess import check_output


def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="""
            Linter to check that commit messages have a new line before
            the body or no body at all.
        """,
        epilog=f"""
            You can manually set the commit-range with the COMMIT_RANGE
            environment variable (e.g. "COMMIT_RANGE='HEAD~n..HEAD'
            {sys.argv[0]}") for the last 'n' commits.
        """)
    return parser.parse_args()


def main():
    parse_args()
    exit_code = 0

    assert os.getenv("COMMIT_RANGE")  # E.g. COMMIT_RANGE='HEAD~n..HEAD'
    commit_range = os.getenv("COMMIT_RANGE")

    commit_hashes = check_output(["git", "log", commit_range, "--format=%H"], text=True, encoding="utf8").splitlines()

    for hash in commit_hashes:
        commit_info = check_output(["git", "log", "--format=%B", "-n", "1", hash], text=True, encoding="utf8").splitlines()
        if len(commit_info) >= 2:
            if commit_info[1]:
                print(f"The subject line of commit hash {hash} is followed by a non-empty line. Subject lines should always be followed by a blank line.")
                exit_code = 1

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
