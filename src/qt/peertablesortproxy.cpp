// Copyright (c) 2020-2022 The Bitcoin Core developers
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

#include <qt/peertablesortproxy.h>

#include <qt/peertablemodel.h>
#include <util/check.h>

#include <QModelIndex>
#include <QString>
#include <QVariant>

PeerTableSortProxy::PeerTableSortProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool PeerTableSortProxy::lessThan(const QModelIndex& left_index, const QModelIndex& right_index) const
{
    const CNodeStats left_stats = Assert(sourceModel()->data(left_index, PeerTableModel::StatsRole).value<CNodeCombinedStats*>())->nodeStats;
    const CNodeStats right_stats = Assert(sourceModel()->data(right_index, PeerTableModel::StatsRole).value<CNodeCombinedStats*>())->nodeStats;

    switch (static_cast<PeerTableModel::ColumnIndex>(left_index.column())) {
    case PeerTableModel::NetNodeId:
        return left_stats.nodeid < right_stats.nodeid;
    case PeerTableModel::Age:
        return left_stats.m_connected > right_stats.m_connected;
    case PeerTableModel::Address:
        return left_stats.m_addr_name.compare(right_stats.m_addr_name) < 0;
    case PeerTableModel::Direction:
        return left_stats.fInbound > right_stats.fInbound; // default sort Inbound, then Outbound
    case PeerTableModel::ConnectionType:
        return left_stats.m_conn_type < right_stats.m_conn_type;
    case PeerTableModel::Network:
        return left_stats.m_network < right_stats.m_network;
    case PeerTableModel::Ping:
        return left_stats.m_min_ping_time < right_stats.m_min_ping_time;
    case PeerTableModel::Sent:
        return left_stats.nSendBytes < right_stats.nSendBytes;
    case PeerTableModel::Received:
        return left_stats.nRecvBytes < right_stats.nRecvBytes;
    case PeerTableModel::Subversion:
        return left_stats.cleanSubVer.compare(right_stats.cleanSubVer) < 0;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}
