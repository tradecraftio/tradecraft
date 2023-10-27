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

#ifndef FREICOIN_QT_OPTIONSMODEL_H
#define FREICOIN_QT_OPTIONSMODEL_H

#include <cstdint>
#include <qt/guiconstants.h>

#include <QAbstractListModel>

#include <assert.h>

namespace interfaces {
class Node;
}

extern const char *DEFAULT_GUI_PROXY_HOST;
static constexpr uint16_t DEFAULT_GUI_PROXY_PORT = 9050;

/**
 * Convert configured prune target MiB to displayed GB. Round up to avoid underestimating max disk usage.
 */
static inline int PruneMiBtoGB(int64_t mib) { return (mib * 1024 * 1024 + GB_BYTES - 1) / GB_BYTES; }

/**
 * Convert displayed prune target GB to configured MiB. Round down so roundtrip GB -> MiB -> GB conversion is stable.
 */
static inline int64_t PruneGBtoMiB(int gb) { return gb * GB_BYTES / 1024 / 1024; }

/** Interface from Qt to configuration data structure for Freicoin client.
   To Qt, the options are presented as a list with the different options
   laid out vertically.
   This can be changed to a tree once the settings become sufficiently
   complex.
 */
class OptionsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit OptionsModel(QObject *parent = nullptr, bool resetSettings = false);

    enum OptionID {
        StartAtStartup,         // bool
        ShowTrayIcon,           // bool
        MinimizeToTray,         // bool
        MapPortUPnP,            // bool
        MapPortNatpmp,          // bool
        MinimizeOnClose,        // bool
        ProxyUse,               // bool
        ProxyIP,                // QString
        ProxyPort,              // int
        ProxyUseTor,            // bool
        ProxyIPTor,             // QString
        ProxyPortTor,           // int
        DisplayUnit,            // FreicoinUnits::Unit
        ThirdPartyTxUrls,       // QString
        Language,               // QString
        UseEmbeddedMonospacedFont, // bool
        CoinControlFeatures,    // bool
        SubFeeFromAmount,       // bool
        ThreadsScriptVerif,     // int
        Prune,                  // bool
        PruneSize,              // int
        DatabaseCache,          // int
        ExternalSignerPath,     // QString
        SpendZeroConfChange,    // bool
        Listen,                 // bool
        Server,                 // bool
        EnablePSFRControls,     // bool
        OptionIDRowCount,
    };

    void Init(bool resetSettings = false);
    void Reset();

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    /** Updates current unit in memory, settings and emits displayUnitChanged(newUnit) signal */
    void setDisplayUnit(const QVariant &value);

    /* Explicit getters */
    bool getShowTrayIcon() const { return m_show_tray_icon; }
    bool getMinimizeToTray() const { return fMinimizeToTray; }
    bool getMinimizeOnClose() const { return fMinimizeOnClose; }
    int getDisplayUnit() const { return nDisplayUnit; }
    QString getThirdPartyTxUrls() const { return strThirdPartyTxUrls; }
    bool getUseEmbeddedMonospacedFont() const { return m_use_embedded_monospaced_font; }
    bool getCoinControlFeatures() const { return fCoinControlFeatures; }
    bool getSubFeeFromAmount() const { return m_sub_fee_from_amount; }
    bool getEnablePSFRControls() const { return m_enable_pst_controls; }
    const QString& getOverriddenByCommandLine() { return strOverriddenByCommandLine; }

    /* Explicit setters */
    void SetPruneEnabled(bool prune, bool force = false);
    void SetPruneTargetGB(int prune_target_gb, bool force = false);

    /* Restart flag helper */
    void setRestartRequired(bool fRequired);
    bool isRestartRequired() const;

    interfaces::Node& node() const { assert(m_node); return *m_node; }
    void setNode(interfaces::Node& node) { assert(!m_node); m_node = &node; }

private:
    interfaces::Node* m_node = nullptr;
    /* Qt-only settings */
    bool m_show_tray_icon;
    bool fMinimizeToTray;
    bool fMinimizeOnClose;
    QString language;
    int nDisplayUnit;
    QString strThirdPartyTxUrls;
    bool m_use_embedded_monospaced_font;
    bool fCoinControlFeatures;
    bool m_sub_fee_from_amount;
    bool m_enable_pst_controls;
    /* settings that were overridden by command-line */
    QString strOverriddenByCommandLine;

    // Add option to list of GUI options overridden through command line/config file
    void addOverriddenOption(const std::string &option);

    // Check settings version and upgrade default values if required
    void checkAndMigrate();
Q_SIGNALS:
    void displayUnitChanged(int unit);
    void coinControlFeaturesChanged(bool);
    void showTrayIconChanged(bool);
    void useEmbeddedMonospacedFontChanged(bool);
};

#endif // FREICOIN_QT_OPTIONSMODEL_H
