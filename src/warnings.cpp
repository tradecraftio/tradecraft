// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
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

#include "sync.h"
#include "clientversion.h"
#include "util.h"
#include "warnings.h"

CCriticalSection cs_warnings;
std::string strMiscWarning;
bool fLargeWorkForkFound = false;
bool fLargeWorkInvalidChainFound = false;

void SetMiscWarning(const std::string& strWarning)
{
    LOCK(cs_warnings);
    strMiscWarning = strWarning;
}

void SetfLargeWorkForkFound(bool flag)
{
    LOCK(cs_warnings);
    fLargeWorkForkFound = flag;
}

bool GetfLargeWorkForkFound()
{
    LOCK(cs_warnings);
    return fLargeWorkForkFound;
}

void SetfLargeWorkInvalidChainFound(bool flag)
{
    LOCK(cs_warnings);
    fLargeWorkInvalidChainFound = flag;
}

bool GetfLargeWorkInvalidChainFound()
{
    LOCK(cs_warnings);
    return fLargeWorkInvalidChainFound;
}

std::string GetWarnings(const std::string& strFor)
{
    std::string strStatusBar;
    std::string strRPC;
    std::string strGUI;
    const std::string uiAlertSeperator = "<hr />";

    LOCK(cs_warnings);

    if (!CLIENT_VERSION_IS_RELEASE) {
        strStatusBar = "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications";
        strGUI = _("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications");
    }

    if (GetBoolArg("-testsafemode", DEFAULT_TESTSAFEMODE))
        strStatusBar = strRPC = strGUI = "testsafemode enabled";

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        strStatusBar = strMiscWarning;
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + strMiscWarning;
    }

    if (fLargeWorkForkFound)
    {
        strStatusBar = strRPC = "Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.";
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.");
    }
    else if (fLargeWorkInvalidChainFound)
    {
        strStatusBar = strRPC = "Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.";
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.");
    }

    if (strFor == "gui")
        return strGUI;
    else if (strFor == "statusbar")
        return strStatusBar;
    else if (strFor == "rpc")
        return strRPC;
    assert(!"GetWarnings(): invalid parameter");
    return "error";
}
