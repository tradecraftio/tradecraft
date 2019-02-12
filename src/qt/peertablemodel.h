// Copyright (c) 2011-2013 The Bitcoin developers
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

#ifndef BITCOIN_QT_PEERTABLEMODEL_H
#define BITCOIN_QT_PEERTABLEMODEL_H

#include "main.h"
#include "net.h"

#include <QAbstractTableModel>
#include <QStringList>

class ClientModel;
class PeerTablePriv;

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

struct CNodeCombinedStats {
    CNodeStats nodeStats;
    CNodeStateStats nodeStateStats;
    bool fNodeStateStatsAvailable;
};

class NodeLessThan
{
public:
    NodeLessThan(int nColumn, Qt::SortOrder fOrder) :
        column(nColumn), order(fOrder) {}
    bool operator()(const CNodeCombinedStats &left, const CNodeCombinedStats &right) const;

private:
    int column;
    Qt::SortOrder order;
};

/**
   Qt model providing information about connected peers, similar to the
   "getpeerinfo" RPC call. Used by the rpc console UI.
 */
class PeerTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit PeerTableModel(ClientModel *parent = 0);
    const CNodeCombinedStats *getNodeStats(int idx);
    int getRowByNodeId(NodeId nodeid);
    void startAutoRefresh();
    void stopAutoRefresh();

    enum ColumnIndex {
        Address = 0,
        Subversion = 1,
        Ping = 2
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void sort(int column, Qt::SortOrder order);
    /*@}*/

public slots:
    void refresh();

private:
    ClientModel *clientModel;
    QStringList columns;
    PeerTablePriv *priv;
    QTimer *timer;
};

#endif // BITCOIN_QT_PEERTABLEMODEL_H
