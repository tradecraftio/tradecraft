#!/usr/bin/env python3

# Copyright (c) 2011-2021 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>

substitutions = [
# bash and Python source files. Some of the modificatiosn made are
# drawn from other projects done by the copyright holders that date
# back as far as 2010.
    ["""
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
# Copyright (c) 2010-2021 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>
"""],
    ["""
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
# Copyright (c) 2010-2021 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>
"""],
    ["""
Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
Copyright (c) 2010-2021 The Freicoin Developers

This program is free software: you can redistribute it and/or modify
it under the conjunctive terms of BOTH version 3 of the GNU Affero
General Public License as published by the Free Software Foundation
AND the MIT/X11 software license.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Affero General Public License and the MIT/X11 software license for
more details.

You should have received a copy of both licenses along with this
program.  If not, see <https://www.gnu.org/licenses/> and
<http://www.opensource.org/licenses/mit-license.php>
"""],

# Manpages and other ditsribution files
    ["""

Please contribute if you find Freicoin useful. Visit
<https://freico.in> for further information about the software.
The source code is available from <https://github.com/freicoin/freicoin>.

This is experimental software.
Distributed under the MIT software license, see the accompanying file COPYING
or <http://www.opensource.org/licenses/mit-license.php>.
""","""
Copyright (C) 2011-2021 The Freicoin Developers

Please contribute if you find Freicoin useful. Visit
<https://freico.in> for further information about the software.
The source code is available from <https://github.com/freicoin/freicoin>.

This is experimental software.

This program is free software: you can redistribute it and/or modify
it under the conjunctive terms of BOTH version 3 of the GNU Affero
General Public License as published by the Free Software Foundation
AND the MIT/X11 software license.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Affero General Public License and the MIT/X11 software license for
more details.

You should have received a copy of both licenses along with this
program.  If not, see <https://www.gnu.org/licenses/> and
<http://www.opensource.org/licenses/mit-license.php>
"""],

# Embedded comments in media files
    ["""
\tDistributed under the MIT software license, see the accompanying
\tfile COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
\tCopyright (c) 2011-2021 The Freicoin Developers

\tThis program is free software: you can redistribute it and/or modify
\tit under the conjunctive terms of BOTH version 3 of the GNU Affero
\tGeneral Public License as published by the Free Software Foundation
\tAND the MIT/X11 software license.

\tThis program is distributed in the hope that it will be useful, but
\tWITHOUT ANY WARRANTY; without even the implied warranty of
\tMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
\tAffero General Public License and the MIT/X11 software license for
\tmore details.

\tYou should have received a copy of both licenses along with this
\tprogram.  If not, see <https://www.gnu.org/licenses/> and
\t<http://www.opensource.org/licenses/mit-license.php>
"""],

# C/C++ source files. Work on the Freicoin fork of Freicoin started in
# 2011, and the current codebase is a direct continuation of that.
    ["""
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
// Copyright (c) 2011-2021 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>
"""],
    ["""
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
// Copyright (c) 2011-2021 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>
"""],

# json_spirt ha no known modifications prior to 2019.
    ["""
//          Copyright John W. Wilkinson 2007 - 2009.
// Distributed under the MIT License, see accompanying file LICENSE.txt
""","""
//          Copyright The Freicoin Developers 2019-2021.
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>
"""],

# libsecp256k1 source files have their own coypright declaration
# format. We can be more restrictive in saying that modifications to
# libsecp256k1, specifically, didn't start until 2018.
    ["""
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
""","""
 * Copyright (c) 2018-2021 The Freicoin Developers                    *
 *                                                                    *
 * This program is free software: you can redistribute it and/or      *
 * modify it under the conjunctive terms of BOTH version 3 of the GNU *
 * Affero General Public License as published by the Free Software    *
 * Foundation AND the MIT/X11 software license.                       *
 *                                                                    *
 * This program is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 * Affero General Public License and the MIT/X11 software license for *
 * more details.                                                      *
 *                                                                    *
 * You should have received a copy of both licenses along with this   *
 * program.  If not, see <https://www.gnu.org/licenses/> and          *
 * <http://www.opensource.org/licenses/mit-license.php>               *
"""],

# Some are 1 character wider:
    ["""
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php. *
""","""
 * Copyright (c) 2018-2021 The Freicoin Developers                     *
 *                                                                     *
 * This program is free software: you can redistribute it and/or       *
 * modify it under the conjunctive terms of BOTH version 3 of the GNU  *
 * Affero General Public License as published by the Free Software     *
 * Foundation AND the MIT/X11 software license.                        *
 *                                                                     *
 * This program is distributed in the hope that it will be useful,     *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Affero General Public License and the MIT/X11 software license for  *
 * more details.                                                       *
 *                                                                     *
 * You should have received a copy of both licenses along with this    *
 * program.  If not, see <https://www.gnu.org/licenses/> and           *
 * <http://www.opensource.org/licenses/mit-license.php>                *
"""],

# Various build scripts which date from no earlier than 2013.
    ["""
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
dnl Copyright (c) 2013-2021 The Freicoin Developers
dnl
dnl This program is free software: you can redistribute it and/or
dnl modify it under the conjunctive terms of BOTH version 3 of the GNU
dnl Affero General Public License as published by the Free Software
dnl Foundation AND the MIT/X11 software license.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Affero General Public License and the MIT/X11 software license for
dnl more details.
dnl
dnl You should have received a copy of both licenses along with this
dnl program.  If not, see <https://www.gnu.org/licenses/> and
dnl <http://www.opensource.org/licenses/mit-license.php>
"""],
]

import sys
assert(len(sys.argv)==2)

if __name__ == "__main__":
    text = open(sys.argv[1], "r").read();
    for args in substitutions:
        text = text.replace(*args)
    open(sys.argv[1], "w").write(text)

# End of File
