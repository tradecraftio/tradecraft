#!/usr/bin/env python3
#
# Copyright (c) 2022 The Bitcoin Core developers
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

"""
Find dead Python code.
"""

from subprocess import check_output, STDOUT, CalledProcessError

FILES_ARGS = ['git', 'ls-files', '--', '*.py']


def check_vulture_install():
    try:
        check_output(["vulture", "--version"])
    except FileNotFoundError:
        print("Skipping Python dead code linting since vulture is not installed. Install by running \"pip3 install vulture\"")
        exit(0)


def main():
    check_vulture_install()

    files = check_output(FILES_ARGS).decode("utf-8").splitlines()
    # --min-confidence 100 will only report code that is guaranteed to be unused within the analyzed files.
    # Any value below 100 introduces the risk of false positives, which would create an unacceptable maintenance burden.
    vulture_args = ['vulture', '--min-confidence=100'] + files

    try:
        check_output(vulture_args, stderr=STDOUT)
    except CalledProcessError as e:
        print(e.output.decode("utf-8"), end="")
        print("Python dead code detection found some issues")
        exit(1)


if __name__ == "__main__":
    main()
