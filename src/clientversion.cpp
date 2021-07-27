// Copyright (c) 2012-2017 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <tinyformat.h>


/**
 * Name of client reported in the 'version' message. Report the same name
 * for both freicoind and freicoin-qt, to make it harder for attackers to
 * target servers or GUI users specifically.
 */
const std::string CLIENT_NAME("Satoshi");

/**
 * Client version number
 */
#define CLIENT_VERSION_SUFFIX ""


/**
 * The following part of the code determines the CLIENT_BUILD variable.
 * Several mechanisms are used for this:
 * * first, if HAVE_BUILD_INFO is defined, include build.h, a file that is
 *   generated by the build environment, possibly containing the output
 *   of git-describe in a macro called BUILD_DESC
 * * secondly, if this is an exported version of the code, GIT_ARCHIVE will
 *   be defined (automatically using the export-subst git attribute), and
 *   GIT_COMMIT will contain the commit id.
 * * then, three options exist for determining CLIENT_BUILD:
 *   * if BUILD_DESC is defined, use that literally (output of git-describe)
 *   * if not, but GIT_COMMIT is defined, use v[maj].[min].[rev].[build]-g[commit]
 *   * otherwise, use v[maj].[min].[rev].[build]-unk
 * finally CLIENT_VERSION_SUFFIX is added
 */

//! First, include build.h if requested
#ifdef HAVE_BUILD_INFO
#include <obj/build.h>
#endif

//! git will put "#define GIT_ARCHIVE 1" on the next line inside archives. $Format:%n#define GIT_ARCHIVE 1$
#ifdef GIT_ARCHIVE
#define GIT_COMMIT_ID "$Format:%h$"
#define GIT_COMMIT_DATE "$Format:%cD$"
#endif

#define BUILD_DESC_WITH_SUFFIX(base, build, suffix) \
    "v" DO_STRINGIZE(base) "-" DO_STRINGIZE(build) "-" DO_STRINGIZE(suffix)

#define BUILD_DESC_FROM_COMMIT(base, build, commit) \
    "v" DO_STRINGIZE(base) "-" DO_STRINGIZE(build) "-g" commit

#define BUILD_DESC_FROM_UNKNOWN(base, build) \
    "v" DO_STRINGIZE(base) "-" DO_STRINGIZE(build) "-unk"

#ifndef BUILD_DESC
#ifdef BUILD_SUFFIX
#define BUILD_DESC BUILD_DESC_WITH_SUFFIX(CLIENT_VERSION_BASE_STRING, CLIENT_VERSION_BUILD, BUILD_SUFFIX)
#elif defined(GIT_COMMIT_ID)
#define BUILD_DESC BUILD_DESC_FROM_COMMIT(CLIENT_VERSION_BASE_STRING, CLIENT_VERSION_BUILD, GIT_COMMIT_ID)
#else
#define BUILD_DESC BUILD_DESC_FROM_UNKNOWN(CLIENT_VERSION_BASE_STRING, CLIENT_VERSION_BUILD)
#endif
#endif

const std::string CLIENT_BUILD(BUILD_DESC CLIENT_VERSION_SUFFIX);

static std::string FormatVersion(int nVersion)
{
    if (nVersion % 1000000 == 0)
        return strprintf("%d", nVersion / 1000000);
    if (nVersion % 10000 == 0)
        return strprintf("%d.%d", nVersion / 1000000, (nVersion / 10000) % 100);
    if (nVersion % 100 == 0)
        return strprintf("%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100, (nVersion / 100) % 100);
    else
        return strprintf("%d.%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100, (nVersion / 100) % 100, nVersion % 100);
}

std::string FormatFullVersion()
{
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
