#!/usr/bin/env bash
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
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
# Make sure all shell scripts:
# a.) explicitly opt out of locale dependence using
#     "export LC_ALL=C" or "export LC_ALL=C.UTF-8", or
# b.) explicitly opt in to locale dependence using the annotation below.

export LC_ALL=C

EXIT_CODE=0
for SHELL_SCRIPT in $(git ls-files -- "*.sh" | grep -vE "src/(secp256k1|minisketch|univalue)/"); do
    if grep -q "# This script is intentionally locale dependent by not setting \"export LC_ALL=C\"" "${SHELL_SCRIPT}"; then
        continue
    fi
    FIRST_NON_COMMENT_LINE=$(grep -vE '^(#.*)?$' "${SHELL_SCRIPT}" | head -1)
    if [[ ${FIRST_NON_COMMENT_LINE} != "export LC_ALL=C" && ${FIRST_NON_COMMENT_LINE} != "export LC_ALL=C.UTF-8" ]]; then
        echo "Missing \"export LC_ALL=C\" (to avoid locale dependence) as first non-comment non-empty line in ${SHELL_SCRIPT}"
        EXIT_CODE=1
    fi
done
exit ${EXIT_CODE}
