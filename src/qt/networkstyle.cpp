// Copyright (c) 2014 The Bitcoin developers
// Copyright (c) 2011-2019 The Freicoin developers
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

#include "networkstyle.h"

#include "guiconstants.h"

#include <QApplication>

static const struct {
    const char *networkId;
    const char *appName;
    const char *appIcon;
    const char *titleAddText;
    const char *splashImage;
} network_styles[] = {
    {"main", QAPP_APP_NAME_DEFAULT, ":/icons/freicoin", "", ":/images/splash"},
    {"test", QAPP_APP_NAME_TESTNET, ":/icons/freicoin_testnet", QT_TRANSLATE_NOOP("SplashScreen", "[testnet]"), ":/images/splash_testnet"},
    {"regtest", QAPP_APP_NAME_TESTNET, ":/icons/freicoin_testnet", "[regtest]", ":/images/splash_testnet"}
};
static const unsigned network_styles_count = sizeof(network_styles)/sizeof(*network_styles);

// titleAddText needs to be const char* for tr()
NetworkStyle::NetworkStyle(const QString &appName, const QString &appIcon, const char *titleAddText, const QString &splashImage):
    appName(appName),
    appIcon(appIcon),
    titleAddText(qApp->translate("SplashScreen", titleAddText)),
    splashImage(splashImage)
{
}

const NetworkStyle *NetworkStyle::instantiate(const QString &networkId)
{
    for (unsigned x=0; x<network_styles_count; ++x)
    {
        if (networkId == network_styles[x].networkId)
        {
            return new NetworkStyle(
                    network_styles[x].appName,
                    network_styles[x].appIcon,
                    network_styles[x].titleAddText,
                    network_styles[x].splashImage);
        }
    }
    return 0;
}
