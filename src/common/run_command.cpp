// Copyright (c) 2022 The Bitcoin Core developers
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

#if defined(HAVE_CONFIG_H)
#include <config/freicoin-config.h>
#endif

#include <common/run_command.h>

#include <tinyformat.h>
#include <univalue.h>

#ifdef ENABLE_EXTERNAL_SIGNER
#if defined(__GNUC__)
// Boost 1.78 requires the following workaround.
// See: https://github.com/boostorg/process/issues/235
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
#endif
#include <boost/process.hpp>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif // ENABLE_EXTERNAL_SIGNER

UniValue RunCommandParseJSON(const std::string& str_command, const std::string& str_std_in)
{
#ifdef ENABLE_EXTERNAL_SIGNER
    namespace bp = boost::process;

    UniValue result_json;
    bp::opstream stdin_stream;
    bp::ipstream stdout_stream;
    bp::ipstream stderr_stream;

    if (str_command.empty()) return UniValue::VNULL;

    bp::child c(
        str_command,
        bp::std_out > stdout_stream,
        bp::std_err > stderr_stream,
        bp::std_in < stdin_stream
    );
    if (!str_std_in.empty()) {
        stdin_stream << str_std_in << std::endl;
    }
    stdin_stream.pipe().close();

    std::string result;
    std::string error;
    std::getline(stdout_stream, result);
    std::getline(stderr_stream, error);

    c.wait();
    const int n_error = c.exit_code();
    if (n_error) throw std::runtime_error(strprintf("RunCommandParseJSON error: process(%s) returned %d: %s\n", str_command, n_error, error));
    if (!result_json.read(result)) throw std::runtime_error("Unable to parse JSON: " + result);

    return result_json;
#else
    throw std::runtime_error("Compiled without external signing support (required for external signing).");
#endif // ENABLE_EXTERNAL_SIGNER
}
