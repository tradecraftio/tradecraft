// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#ifndef FREICOIN_QT_FREICOINAMOUNTFIELD_H
#define FREICOIN_QT_FREICOINAMOUNTFIELD_H

#include <consensus/amount.h>
#include <qt/freicoinunits.h>

#include <QWidget>

class AmountSpinBox;

QT_BEGIN_NAMESPACE
class QValueComboBox;
QT_END_NAMESPACE

/** Widget for entering freicoin amounts.
  */
class FreicoinAmountField: public QWidget
{
    Q_OBJECT

    // ugly hack: for some unknown reason CAmount (instead of qint64) does not work here as expected
    // discussion: https://github.com/bitcoin/bitcoin/pull/5117
    Q_PROPERTY(qint64 value READ value WRITE setValue NOTIFY valueChanged USER true)

public:
    explicit FreicoinAmountField(QWidget *parent = nullptr);

    CAmount value(bool *value=nullptr) const;
    void setValue(const CAmount& value);

    /** If allow empty is set to false the field will be set to the minimum allowed value if left empty. **/
    void SetAllowEmpty(bool allow);

    /** Set the minimum value in kria **/
    void SetMinValue(const CAmount& value);

    /** Set the maximum value in kria **/
    void SetMaxValue(const CAmount& value);

    /** Set single step in kria **/
    void setSingleStep(const CAmount& step);

    /** Make read-only **/
    void setReadOnly(bool fReadOnly);

    /** Mark current value as invalid in UI. */
    void setValid(bool valid);
    /** Perform input validation, mark field as invalid if entered value is not valid. */
    bool validate();

    /** Change unit used to display amount. */
    void setDisplayUnit(FreicoinUnit new_unit);

    /** Make field empty and ready for new input. */
    void clear();

    /** Enable/Disable. */
    void setEnabled(bool fEnabled);

    /** Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907),
        in these cases we have to set it up manually.
    */
    QWidget *setupTabChain(QWidget *prev);

Q_SIGNALS:
    void valueChanged();

protected:
    /** Intercept focus-in event and ',' key presses */
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    AmountSpinBox* amount{nullptr};
    QValueComboBox *unit;

private Q_SLOTS:
    void unitChanged(int idx);

};

#endif // FREICOIN_QT_FREICOINAMOUNTFIELD_H
