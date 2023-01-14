#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
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

export LC_ALL=C.UTF-8

for b_name in "${BASE_OUTDIR}/bin"/*; do
    # shellcheck disable=SC2044
    for b in $(find "${BASE_ROOT_DIR}" -executable -type f -name "$(basename "$b_name")"); do
      echo "Wrap $b ..."
      mv "$b" "${b}_orig"
      echo '#!/usr/bin/env bash' > "$b"
      echo "valgrind --gen-suppressions=all --quiet --error-exitcode=1 --suppressions=${BASE_ROOT_DIR}/contrib/valgrind.supp \"${b}_orig\" \"\$@\"" >> "$b"
      chmod +x "$b"
    done
done
