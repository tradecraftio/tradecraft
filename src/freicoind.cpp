// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include <chainparams.h>
#include <clientversion.h>
#include <compat.h>
#include <init.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <noui.h>
#include <shutdown.h>
#include <ui_interface.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/threadnames.h>
#include <util/translation.h>

#include <functional>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static void WaitForShutdown(NodeContext& node)
{
    while (!ShutdownRequested())
    {
        UninterruptibleSleep(std::chrono::milliseconds{200});
    }
    Interrupt(node);
}

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
static bool AppInit(int argc, char* argv[])
{
    NodeContext node;
    node.chain = interfaces::MakeChain(node);

    bool fRet = false;

    util::ThreadSetInternalName("init");

    //
    // Parameters
    //
    // If Qt is used, parameters/freicoin.conf are parsed in qt/freicoin.cpp's main()
    SetupServerArgs();
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        return InitError(strprintf("Error parsing command line arguments: %s\n", error));
    }

    // Process help and version before taking care about datadir
    if (HelpRequested(gArgs) || gArgs.IsArgSet("-version")) {
        std::string strUsage = PACKAGE_NAME " version " + FormatFullVersion() + "\n";

        if (gArgs.IsArgSet("-version"))
        {
            strUsage += FormatParagraph(LicenseInfo()) + "\n";
        }
        else
        {
            strUsage += "\nUsage:  freicoind [options]                     Start " PACKAGE_NAME "\n";
            strUsage += "\n" + gArgs.GetHelpMessage();
        }

        tfm::format(std::cout, "%s", strUsage);
        return true;
    }

    try
    {
        if (!CheckDataDirOption()) {
            return InitError(strprintf("Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", "")));
        }
        if (!gArgs.ReadConfigFiles(error, true)) {
            return InitError(strprintf("Error reading configuration file: %s\n", error));
        }
        // Check for -chain, -testnet or -regtest parameter (Params() calls are only valid after this clause)
        try {
            SelectParams(gArgs.GetChainName());
        } catch (const std::exception& e) {
            return InitError(strprintf("%s\n", e.what()));
        }

        // Error out when loose non-argument tokens are encountered on command line
        for (int i = 1; i < argc; i++) {
            if (!IsSwitchChar(argv[i][0])) {
                return InitError(strprintf("Command line contains unexpected token '%s', see freicoind -h for a list of options.\n", argv[i]));
            }
        }

        // -server defaults to true for freicoind but not for the GUI so do this here
        gArgs.SoftSetBoolArg("-server", true);
        // Set this early so that parameter interactions go to console
        InitLogging();
        InitParameterInteraction();
        if (!AppInitBasicSetup())
        {
            // InitError will have been called with detailed error, which ends up on console
            return false;
        }
        if (!AppInitParameterInteraction())
        {
            // InitError will have been called with detailed error, which ends up on console
            return false;
        }
        if (!AppInitSanityChecks())
        {
            // InitError will have been called with detailed error, which ends up on console
            return false;
        }
        if (gArgs.GetBoolArg("-daemon", false))
        {
#if HAVE_DECL_DAEMON
#if defined(MAC_OSX)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
            tfm::format(std::cout, PACKAGE_NAME " starting\n");

            // Daemonize
            if (daemon(1, 0)) { // don't chdir (1), do close FDs (0)
                return InitError(strprintf("daemon() failed: %s\n", strerror(errno)));
            }
#if defined(MAC_OSX)
#pragma GCC diagnostic pop
#endif
#else
            return InitError("-daemon is not supported on this operating system\n");
#endif // HAVE_DECL_DAEMON
        }
        // Lock data directory after daemonization
        if (!AppInitLockDataDirectory())
        {
            // If locking the data directory failed, exit immediately
            return false;
        }
        fRet = AppInitMain(node);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInit()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInit()");
    }

    if (!fRet)
    {
        Interrupt(node);
    } else {
        WaitForShutdown(node);
    }
    Shutdown(node);

    return fRet;
}

int main(int argc, char* argv[])
{
#ifdef WIN32
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif
    SetupEnvironment();

    // Connect freicoind signal handlers
    noui_connect();

    return (AppInit(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE);
}
