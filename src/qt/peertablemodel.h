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

#ifndef BITCOIN_QT_PEERTABLEMODEL_H
#define BITCOIN_QT_PEERTABLEMODEL_H

#include <net_processing.h> // For CNodeStateStats
#include <net.h>

#include <QAbstractTableModel>
#include <QList>
#include <QModelIndex>
#include <QStringList>
#include <QVariant>

class PeerTablePriv;

namespace interfaces {
class Node;
}

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

struct CNodeCombinedStats {
    CNodeStats nodeStats;
    CNodeStateStats nodeStateStats;
    bool fNodeStateStatsAvailable;
};
Q_DECLARE_METATYPE(CNodeCombinedStats*)

/**
   Qt model providing information about connected peers, similar to the
   "getpeerinfo" RPC call. Used by the rpc console UI.
 */
class PeerTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit PeerTableModel(interfaces::Node& node, QObject* parent);
    ~PeerTableModel();
    void startAutoRefresh();
    void stopAutoRefresh();

    enum ColumnIndex {
        NetNodeId = 0,
        Address,
        Direction,
        ConnectionType,
        Network,
        Ping,
        Sent,
        Received,
        Subversion
    };

    enum {
        StatsRole = Qt::UserRole,
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    /*@}*/

public Q_SLOTS:
    void refresh();

private:
    //! Internal peer data structure.
    QList<CNodeCombinedStats> m_peers_data{};
    interfaces::Node& m_node;
    const QStringList columns{
        /*: Title of Peers Table column which contains a
            unique number used to identify a connection. */
        tr("Peer"),
        /*: Title of Peers Table column which contains the
            IP/Onion/I2P address of the connected peer. */
        tr("Address"),
        /*: Title of Peers Table column which indicates the direction
            the peer connection was initiated from. */
        tr("Direction"),
        /*: Title of Peers Table column which describes the type of
            peer connection. The "type" describes why the connection exists. */
        tr("Type"),
        /*: Title of Peers Table column which states the network the peer
            connected through. */
        tr("Network"),
        /*: Title of Peers Table column which indicates the current latency
            of the connection with the peer. */
        tr("Ping"),
        /*: Title of Peers Table column which indicates the total amount of
            network information we have sent to the peer. */
        tr("Sent"),
        /*: Title of Peers Table column which indicates the total amount of
            network information we have received from the peer. */
        tr("Received"),
        /*: Title of Peers Table column which contains the peer's
            User Agent string. */
        tr("User Agent")};
    QTimer *timer;
};

#endif // BITCOIN_QT_PEERTABLEMODEL_H
