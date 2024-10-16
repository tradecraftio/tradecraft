#!/usr/bin/env bash
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

export LC_ALL=C.UTF-8

for b_name in {"${BASE_OUTDIR}/bin"/*,src/secp256k1/*tests,src/minisketch/test{,-verify},src/univalue/{test_json,unitester,object}}.exe; do
    # shellcheck disable=SC2044
    for b in $(find "${BASE_ROOT_DIR}" -executable -type f -name "$(basename "$b_name")"); do
      if (file "$b" | grep "Windows"); then
        echo "Wrap $b ..."
        mv "$b" "${b}_orig"
        echo '#!/usr/bin/env bash' > "$b"
        echo "( wine \"${b}_orig\" \"\$@\" ) || ( sleep 1 && wine \"${b}_orig\" \"\$@\" )" >> "$b"
        chmod +x "$b"
      fi
    done
done
