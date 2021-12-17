#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
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

import argparse

parser = argparse.ArgumentParser(description='Remove the coverage data from a tracefile for all files matching the pattern.')
parser.add_argument('--pattern', '-p', action='append', help='the pattern of files to remove', required=True)
parser.add_argument('tracefile', help='the tracefile to remove the coverage data from')
parser.add_argument('outfile', help='filename for the output to be written to')

args = parser.parse_args()
tracefile = args.tracefile
pattern = args.pattern
outfile = args.outfile

in_remove = False
with open(tracefile, 'r', encoding="utf8") as f:
    with open(outfile, 'w', encoding="utf8") as wf:
        for line in f:
            for p in pattern:
                if line.startswith("SF:") and p in line:
                    in_remove = True
            if not in_remove:
                wf.write(line)
            if line == 'end_of_record\n':
                in_remove = False
