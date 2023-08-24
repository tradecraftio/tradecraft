// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <tinyformat.h>
#include <util/syserror.h>

#include <cstring>
#include <string>

std::string SysErrorString(int err)
{
    char buf[1024];
    /* Too bad there are three incompatible implementations of the
     * thread-safe strerror. */
    const char *s = nullptr;
#ifdef WIN32
    if (strerror_s(buf, sizeof(buf), err) == 0) s = buf;
#else
#ifdef STRERROR_R_CHAR_P /* GNU variant can return a pointer outside the passed buffer */
    s = strerror_r(err, buf, sizeof(buf));
#else /* POSIX variant always returns message in buffer */
    if (strerror_r(err, buf, sizeof(buf)) == 0) s = buf;
#endif
#endif
    if (s != nullptr) {
        return strprintf("%s (%d)", s, err);
    } else {
        return strprintf("Unknown error (%d)", err);
    }
}
