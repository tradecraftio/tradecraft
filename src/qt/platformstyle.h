// Copyright (c) 2015 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
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

#ifndef BITCOIN_QT_PLATFORMSTYLE_H
#define BITCOIN_QT_PLATFORMSTYLE_H

#include <QIcon>
#include <QPixmap>
#include <QString>

/* Coin network-specific GUI style information */
class PlatformStyle
{
public:
    /** Get style associated with provided platform name, or 0 if not known */
    static const PlatformStyle *instantiate(const QString &platformId);

    const QString &getName() const { return name; }

    bool getImagesOnButtons() const { return imagesOnButtons; }
    bool getUseExtraSpacing() const { return useExtraSpacing; }

    QColor TextColor() const { return textColor; }
    QColor SingleColor() const { return singleColor; }

    /** Colorize an image (given filename) with the icon color */
    QImage SingleColorImage(const QString& filename) const;

    /** Colorize an icon (given filename) with the icon color */
    QIcon SingleColorIcon(const QString& filename) const;

    /** Colorize an icon (given object) with the icon color */
    QIcon SingleColorIcon(const QIcon& icon) const;

    /** Colorize an icon (given filename) with the text color */
    QIcon TextColorIcon(const QString& filename) const;

    /** Colorize an icon (given object) with the text color */
    QIcon TextColorIcon(const QIcon& icon) const;

private:
    PlatformStyle(const QString &name, bool imagesOnButtons, bool colorizeIcons, bool useExtraSpacing);

    QString name;
    bool imagesOnButtons;
    bool colorizeIcons;
    bool useExtraSpacing;
    QColor singleColor;
    QColor textColor;
    /* ... more to come later */
};

#endif // BITCOIN_QT_PLATFORMSTYLE_H

