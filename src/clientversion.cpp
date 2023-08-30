// Copyright (c) 2012-2022 The Bitcoin Core developers
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

#include <clientversion.h>
#include <util/translation.h>

#include <tinyformat.h>

#include <sstream>
#include <string>
#include <vector>

/**
 * Name of client reported in the 'version' message. Report the same name
 * for both bitcoind and bitcoin-qt, to make it harder for attackers to
 * target servers or GUI users specifically.
 */
const std::string CLIENT_NAME("Satoshi");


#ifdef HAVE_BUILD_INFO
#include <obj/build.h>
// The <obj/build.h>, which is generated by the build environment (share/genbuild.sh),
// could contain only one line of the following:
//   - "#define BUILD_GIT_TAG ...", if the top commit is tagged
//   - "#define BUILD_GIT_COMMIT ...", if the top commit is not tagged
//   - "// No build information available", if proper git information is not available
#endif

//! git will put "#define GIT_COMMIT_ID ..." on the next line inside archives. $Format:%n#define GIT_COMMIT_ID "%H"$

#ifdef BUILD_GIT_TAG
    #define BUILD_DESC BUILD_GIT_TAG
    #define BUILD_SUFFIX ""
#else
    #define BUILD_DESC "v" STRINGIZE(CLIENT_RELEASE_STRING)
    #if CLIENT_VERSION_IS_RELEASE
        #define BUILD_SUFFIX ""
    #elif defined(BUILD_GIT_COMMIT)
        #define BUILD_SUFFIX "-" BUILD_GIT_COMMIT
    #elif defined(GIT_COMMIT_ID)
        #define BUILD_SUFFIX "-g" GIT_COMMIT_ID
    #else
        #define BUILD_SUFFIX "-unk"
    #endif
#endif

static std::string FormatVersion(int nVersion)
{
    if (nVersion % 10000 == 0) {
        return strprintf("%d", nVersion / 10000);
    } if (nVersion % 100 == 0) {
        return strprintf("%d.%d", nVersion / 10000, (nVersion / 100) % 100);
    } else {
        return strprintf("%d.%d.%d", nVersion / 10000, (nVersion / 100) % 100, nVersion % 100);
    }
}

std::string FormatFullVersion()
{
    static const std::string CLIENT_BUILD(BUILD_DESC BUILD_SUFFIX);
    return CLIENT_BUILD;
}

/**
 * Format the subversion field according to BIP 14 spec (https://github.com/bitcoin/bips/blob/master/bip-0014.mediawiki)
 */
std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments)
{
    std::ostringstream ss;
    ss << "/";
    ss << name << ":" << FormatVersion(nClientVersion);
    if (!comments.empty())
    {
        std::vector<std::string>::const_iterator it(comments.begin());
        ss << "(" << *it;
        for(++it; it != comments.end(); ++it)
            ss << "; " << *it;
        ss << ")";
    }
    ss << "/";
    return ss.str();
}

std::string CopyrightHolders(const std::string& strPrefix)
{
    const auto copyright_devs = strprintf(_(COPYRIGHT_HOLDERS).translated, COPYRIGHT_HOLDERS_SUBSTITUTION);
    std::string strCopyrightHolders = strPrefix + copyright_devs;

    // Make sure Bitcoin Core copyright is not removed by accident
    if (copyright_devs.find("Bitcoin Core") == std::string::npos) {
        strCopyrightHolders += "\n" + strPrefix + "The Bitcoin Core developers";
    }
    return strCopyrightHolders;
}

std::string LicenseInfo()
{
    const std::string URL_SOURCE_CODE = "<https://github.com/bitcoin/bitcoin>";

    return CopyrightHolders(strprintf(_("Copyright (C) %i-%i").translated, 2009, COPYRIGHT_YEAR) + " ") + "\n" +
           "\n" +
           strprintf(_("Please contribute if you find %s useful. "
                       "Visit %s for further information about the software.").translated, PACKAGE_NAME, "<" PACKAGE_URL ">") +
           "\n" +
           strprintf(_("The source code is available from %s.").translated, URL_SOURCE_CODE) +
           "\n" +
           "\n" +
           _("This is experimental software.").translated + "\n" +
           strprintf(_("Distributed under the GNU Affero General Purpose License v3.0, see the accompanying file %s or %s").translated, "COPYING", "<https://www.gnu.org/licenses/agpl-3.0.en.html>") +
           "\n";
}
