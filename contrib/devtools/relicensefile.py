#!/usr/bin/env python3

# Copyright (c) 2011-2023 The Freicoin Developers
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

substitutions = [
# bash and Python source files. Some of the modificatiosn made are
# drawn from other projects done by the copyright holders that date
# back as far as 2010.
    ["""
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
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
"""],
    ["""
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
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
"""],
    ["""
Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
Copyright (c) 2010-2023 The Freicoin Developers

This program is free software: you can redistribute it and/or modify it under
the terms of version 3 of the GNU Affero General Public License as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""],

# Manpages and other ditsribution files
    ["""

Please contribute if you find Bitcoin Core useful. Visit
<https://bitcoincore.org> for further information about the software.
The source code is available from <https://github.com/bitcoin/bitcoin>.

This is experimental software.
Distributed under the MIT software license, see the accompanying file COPYING
or <http://www.opensource.org/licenses/mit-license.php>.
""","""
Copyright (C) 2011-2023 The Freicoin Developers

Please contribute if you find Bitcoin Core useful. Visit
<https://bitcoincore.org> for further information about the software.
The source code is available from <https://github.com/bitcoin/bitcoin>.

This is experimental software.

This program is free software: you can redistribute it and/or modify it under
the terms of version 3 of the GNU Affero General Public License as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""],
    ["""

Please contribute if you find Bitcoin Core useful. Visit
<https://bitcoincore.org> for further information about the software.
The source code is available from <https://github.com/bitcoin/bitcoin>.

This is experimental software.
Distributed under the MIT software license, see the accompanying file COPYING
or <https://opensource.org/licenses/MIT>
""","""
Copyright (C) 2011-2023 The Freicoin Developers

Please contribute if you find Bitcoin Core useful. Visit
<https://bitcoincore.org> for further information about the software.
The source code is available from <https://github.com/bitcoin/bitcoin>.

This is experimental software.

This program is free software: you can redistribute it and/or modify it under
the terms of version 3 of the GNU Affero General Public License as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""],

# Embedded comments in media files
    ["""
\tDistributed under the MIT software license, see the accompanying
\tfile COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
\tCopyright (c) 2011-2023 The Freicoin Developers

\tThis program is free software: you can redistribute it and/or modify it under
\tthe terms of version 3 of the GNU Affero General Public License as published
\tby the Free Software Foundation.

\tThis program is distributed in the hope that it will be useful, but WITHOUT
\tANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
\tFOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
\tdetails.

\tYou should have received a copy of the GNU Affero General Public License
\talong with this program.  If not, see <https://www.gnu.org/licenses/>.
"""],

# C/C++ source files. Work on the Freicoin fork of Bitcoin started in
# 2011, and the current codebase is a direct continuation of that.
    ["""
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
// Copyright (c) 2011-2023 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""],
    ["""
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
// Copyright (c) 2011-2023 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""],

# json_spirt ha no known modifications prior to 2019.
    ["""
//          Copyright John W. Wilkinson 2007 - 2009.
// Distributed under the MIT License, see accompanying file LICENSE.txt
""","""
//          Copyright John W. Wilkinson 2007 - 2009.
//          Copyright The Freicoin Developers 2019-2023.
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""],

# libsecp256k1 source files have their own coypright declaration
# format. We can be more restrictive in saying that modifications to
# libsecp256k1, specifically, didn't start until 2018.
    ["""
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
""","""
 * Copyright (c) 2018-2023 The Freicoin Developers                    *
 *                                                                    *
 * This program is free software: you can redistribute it and/or      *
 * modify it under the terms of version 3 of the GNU Affero General   *
 * Public License as published by the Free Software Foundation.       *
 *                                                                    *
 * This program is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 * Affero General Public License for more details.                    *
 *                                                                    *
 * You should have received a copy of the GNU Affero General Public   *
 * License along with this program.  If not, see                      *
 * <https://www.gnu.org/licenses/>.                                   *
"""],

# Some are wider line width:
    ["""
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php. *
""","""
 * Copyright (c) 2018-2023 The Freicoin Developers                     *
 *                                                                     *
 * This program is free software: you can redistribute it and/or       *
 * modify it under the terms of version 3 of the GNU Affero General    *
 * Public License as published by the Free Software Foundation.        *
 *                                                                     *
 * This program is distributed in the hope that it will be useful,     *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Affero General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Affero General Public    *
 * License along with this program.  If not, see                       *
 * <https://www.gnu.org/licenses/>.                                    *
"""],

    ["""
 * Distributed under the MIT software license, see the accompanying          *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.       *
""","""
 * Copyright (c) 2018-2023 The Freicoin Developers                           *
 *                                                                           *
 * This program is free software: you can redistribute it and/or             *
 * modify it under the terms of version 3 of the GNU Affero General          *
 * Public License as published by the Free Software Foundation.              *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Affero General Public License for more details.                           *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public          *
 * License along with this program.  If not, see                             *
 * <https://www.gnu.org/licenses/>.                                          *
"""],

# Various build scripts which date from no earlier than 2013.
    ["""
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.
""","""
dnl Copyright (c) 2013-2023 The Freicoin Developers
dnl
dnl This program is free software: you can redistribute it and/or modify it
dnl under the terms of version 3 of the GNU Affero General Public License as
dnl published by the Free Software Foundation.
dnl
dnl This program is distributed in the hope that it will be useful, but WITHOUT
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
dnl FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
dnl for more details.
dnl
dnl You should have received a copy of the GNU Affero General Public License
dnl along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""],

# There's a stay mention of MIT license that needs clarifying.
    ["License: MIT","License: AGPL-3.0"],
    ["""
# Released under MIT License
""","""
# Originally released under MIT License,
# Relicensed under the AGPL-3.0
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
