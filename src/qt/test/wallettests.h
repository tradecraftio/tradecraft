#ifndef FREICOIN_QT_TEST_WALLETTESTS_H
#define FREICOIN_QT_TEST_WALLETTESTS_H

#include <QObject>
#include <QTest>

class WalletTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void walletTests();
};

#endif // FREICOIN_QT_TEST_WALLETTESTS_H
