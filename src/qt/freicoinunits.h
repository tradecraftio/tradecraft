// Copyright (c) 2011-2021 The Bitcoin Core developers
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

#ifndef FREICOIN_QT_FREICOINUNITS_H
#define FREICOIN_QT_FREICOINUNITS_H

#include <consensus/amount.h>

#include <QAbstractListModel>
#include <QDataStream>
#include <QString>

// U+2009 THIN SPACE = UTF-8 E2 80 89
#define REAL_THIN_SP_CP 0x2009
#define REAL_THIN_SP_UTF8 "\xE2\x80\x89"

// QMessageBox seems to have a bug whereby it doesn't display thin/hair spaces
// correctly.  Workaround is to display a space in a small font.  If you
// change this, please test that it doesn't cause the parent span to start
// wrapping.
#define HTML_HACK_SP "<span style='white-space: nowrap; font-size: 6pt'> </span>"

// Define THIN_SP_* variables to be our preferred type of thin space
#define THIN_SP_CP   REAL_THIN_SP_CP
#define THIN_SP_UTF8 REAL_THIN_SP_UTF8
#define THIN_SP_HTML HTML_HACK_SP

/** Freicoin unit definitions. Encapsulates parsing and formatting
   and serves as list model for drop-down selection boxes.
*/
class FreicoinUnits: public QAbstractListModel
{
    Q_OBJECT

public:
    explicit FreicoinUnits(QObject *parent);

    /** Freicoin units.
      @note Source: https://en.bitcoin.it/wiki/Units . Please add only sensible ones
     */
    enum class Unit {
        FRC,
        mFRC,
        uFRC,
        SAT
    };
    Q_ENUM(Unit)

    enum class SeparatorStyle
    {
        NEVER,
        STANDARD,
        ALWAYS
    };

    //! @name Static API
    //! Unit conversion and formatting
    ///@{

    //! Get list of units, for drop-down box
    static QList<Unit> availableUnits();
    //! Long name
    static QString longName(Unit unit);
    //! Short name
    static QString shortName(Unit unit);
    //! Longer description
    static QString description(Unit unit);
    //! Number of kria (1e-8) per unit
    static qint64 factor(Unit unit);
    //! Number of decimals left
    static int decimals(Unit unit);
    //! Format as string
    static QString format(Unit unit, const CAmount& amount, bool plussign = false, SeparatorStyle separators = SeparatorStyle::STANDARD, bool justify = false);
    //! Format as string (with unit)
    static QString formatWithUnit(Unit unit, const CAmount& amount, bool plussign = false, SeparatorStyle separators = SeparatorStyle::STANDARD);
    //! Format as HTML string (with unit)
    static QString formatHtmlWithUnit(Unit unit, const CAmount& amount, bool plussign = false, SeparatorStyle separators = SeparatorStyle::STANDARD);
    //! Format as string (with unit) of fixed length to preserve privacy, if it is set.
    static QString formatWithPrivacy(Unit unit, const CAmount& amount, SeparatorStyle separators, bool privacy);
    //! Parse string to coin amount
    static bool parse(Unit unit, const QString& value, CAmount* val_out);
    //! Gets title for amount column including current display unit if optionsModel reference available */
    static QString getAmountColumnTitle(Unit unit);
    ///@}

    //! @name AbstractListModel implementation
    //! List model for unit drop-down selection box.
    ///@{
    enum RoleIndex {
        /** Unit identifier */
        UnitRole = Qt::UserRole
    };
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    ///@}

    static QString removeSpaces(QString text)
    {
        text.remove(' ');
        text.remove(QChar(THIN_SP_CP));
        return text;
    }

    //! Return maximum number of base units (kria)
    static CAmount maxMoney();

private:
    QList<Unit> unitlist;
};
typedef FreicoinUnits::Unit FreicoinUnit;

QDataStream& operator<<(QDataStream& out, const FreicoinUnit& unit);
QDataStream& operator>>(QDataStream& in, FreicoinUnit& unit);

#endif // FREICOIN_QT_FREICOINUNITS_H
