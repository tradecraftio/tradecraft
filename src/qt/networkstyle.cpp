// Copyright (c) 2014-2021 The Bitcoin Core developers
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

#include <qt/networkstyle.h>

#include <qt/guiconstants.h>

#include <tinyformat.h>
#include <util/chaintype.h>

#include <QApplication>

static const struct {
    const ChainType networkId;
    const char *appName;
    const int iconColorHueShift;
    const int iconColorSaturationReduction;
} network_styles[] = {
    {ChainType::MAIN, QAPP_APP_NAME_DEFAULT, 0, 0},
    {ChainType::TESTNET, QAPP_APP_NAME_TESTNET, 70, 30},
    {ChainType::SIGNET, QAPP_APP_NAME_SIGNET, 35, 15},
    {ChainType::REGTEST, QAPP_APP_NAME_REGTEST, 160, 30},
};

// titleAddText needs to be const char* for tr()
NetworkStyle::NetworkStyle(const QString &_appName, const int iconColorHueShift, const int iconColorSaturationReduction, const char *_titleAddText):
    appName(_appName),
    titleAddText(qApp->translate("SplashScreen", _titleAddText))
{
    // load pixmap
    QPixmap pixmap(":/icons/freicoin");

    if(iconColorHueShift != 0 && iconColorSaturationReduction != 0)
    {
        // generate QImage from QPixmap
        QImage img = pixmap.toImage();

        int h,s,l,a;

        // traverse though lines
        for(int y=0;y<img.height();y++)
        {
            QRgb *scL = reinterpret_cast< QRgb *>( img.scanLine( y ) );

            // loop through pixels
            for(int x=0;x<img.width();x++)
            {
                // preserve alpha because QColor::getHsl doesn't return the alpha value
                a = qAlpha(scL[x]);
                QColor col(scL[x]);

                // get hue value
                col.getHsl(&h,&s,&l);

                // rotate color on RGB color circle
                // 70° should end up with the typical "testnet" green
                h+=iconColorHueShift;

                // change saturation value
                if(s>iconColorSaturationReduction)
                {
                    s -= iconColorSaturationReduction;
                }
                col.setHsl(h,s,l,a);

                // set the pixel
                scL[x] = col.rgba();
            }
        }

        //convert back to QPixmap
        pixmap.convertFromImage(img);
    }

    appIcon             = QIcon(pixmap);
    trayAndWindowIcon   = QIcon(pixmap.scaled(QSize(256,256)));
}

const NetworkStyle* NetworkStyle::instantiate(const ChainType networkId)
{
    std::string titleAddText = networkId == ChainType::MAIN ? "" : strprintf("[%s]", ChainTypeToString(networkId));
    for (const auto& network_style : network_styles) {
        if (networkId == network_style.networkId) {
            return new NetworkStyle(
                    network_style.appName,
                    network_style.iconColorHueShift,
                    network_style.iconColorSaturationReduction,
                    titleAddText.c_str());
        }
    }
    return nullptr;
}
