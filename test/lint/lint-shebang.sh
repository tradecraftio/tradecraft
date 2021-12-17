#!/usr/bin/env bash
# Copyright (c) 2018 The Bitcoin Core developers
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

# Assert expected shebang lines

export LC_ALL=C
EXIT_CODE=0
for PYTHON_FILE in $(git ls-files -- "*.py"); do
    if [[ $(head -c 2 "${PYTHON_FILE}") == "#!" &&
          $(head -n 1 "${PYTHON_FILE}") != "#!/usr/bin/env python3" ]]; then
        echo "Missing shebang \"#!/usr/bin/env python3\" in ${PYTHON_FILE} (do not use python or python2)"
        EXIT_CODE=1
    fi
done
for SHELL_FILE in $(git ls-files -- "*.sh"); do
    if [[ $(head -n 1 "${SHELL_FILE}") != "#!/usr/bin/env bash" &&
          $(head -n 1 "${SHELL_FILE}") != "#!/bin/sh" ]]; then
        echo "Missing expected shebang \"#!/usr/bin/env bash\" or \"#!/bin/sh\" in ${SHELL_FILE}"
        EXIT_CODE=1
    fi
done
exit ${EXIT_CODE}
