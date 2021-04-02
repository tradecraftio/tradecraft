#!/usr/bin/env python
# Copyright 2014 BitPay Inc.
# Copyright 2016 The Bitcoin Core developers
# Copyright (c) 2010-2021 The Freicoin Developers
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
from __future__ import division,print_function,unicode_literals
import os
import bctest
import buildenv
import argparse
import logging

help_text="""Test framework for freicoin utils.

Runs automatically during `make check`.

Can also be run manually from the src directory by specifying the source directory:

test/freicoin-util-test.py --srcdir='srcdir' [--verbose]
"""

if __name__ == '__main__':
    # Try to get the source directory from the environment variables. This will
    # be set for `make check` automated runs. If environment variable is not set,
    # then get the source directory from command line args.
    try:
        srcdir = os.environ["srcdir"]
        verbose = False
    except:
        parser = argparse.ArgumentParser(description=help_text)
        parser.add_argument('-s', '--srcdir')
        parser.add_argument('-v', '--verbose', action='store_true')
        args = parser.parse_args()
        srcdir = args.srcdir
        verbose = args.verbose

    if verbose:
        level = logging.DEBUG
    else:
        level = logging.ERROR
    formatter = '%(asctime)s - %(levelname)s - %(message)s'
    # Add the format/level to the logger
    logging.basicConfig(format = formatter, level=level)

    bctest.bctester(srcdir + "/test/data", "freicoin-util-test.json", buildenv)
