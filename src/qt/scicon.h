// Copyright (c) 2014 The Bitcoin developers
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

#ifndef FREICOIN_QT_SCICON_H
#define FREICOIN_QT_SCICON_H

#include <QtCore>

QT_BEGIN_NAMESPACE
class QColor;
class QIcon;
class QString;
QT_END_NAMESPACE

QImage SingleColorImage(const QString& filename, const QColor&);
QIcon SingleColorIcon(const QIcon&, const QColor&);
QIcon SingleColorIcon(const QString& filename, const QColor&);
QColor SingleColor();
QIcon SingleColorIcon(const QString& filename);
QIcon TextColorIcon(const QIcon&);
QIcon TextColorIcon(const QString& filename);

#endif // FREICOIN_QT_SCICON_H
