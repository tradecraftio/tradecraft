// Copyright (c) 2023 The Bitcoin Core developers
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

#ifndef BITCOIN_COMMON_INIT_H
#define BITCOIN_COMMON_INIT_H

#include <util/translation.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

class ArgsManager;

namespace common {
enum class ConfigStatus {
    FAILED,       //!< Failed generically.
    FAILED_WRITE, //!< Failed to write settings.json
    ABORTED,      //!< Aborted by user
};

struct ConfigError {
    ConfigStatus status;
    bilingual_str message{};
    std::vector<std::string> details{};
};

//! Callback function to let the user decide whether to abort loading if
//! settings.json file exists and can't be parsed, or to ignore the error and
//! overwrite the file.
using SettingsAbortFn = std::function<bool(const bilingual_str& message, const std::vector<std::string>& details)>;

/* Read config files, and create datadir and settings.json if they don't exist. */
std::optional<ConfigError> InitConfig(ArgsManager& args, SettingsAbortFn settings_abort_fn = nullptr);
} // namespace common

#endif // BITCOIN_COMMON_INIT_H
