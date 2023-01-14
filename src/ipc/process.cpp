// Copyright (c) 2021 The Bitcoin Core developers
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

#include <fs.h>
#include <ipc/process.h>
#include <ipc/protocol.h>
#include <mp/util.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

namespace ipc {
namespace {
class ProcessImpl : public Process
{
public:
    int spawn(const std::string& new_exe_name, const fs::path& argv0_path, int& pid) override
    {
        return mp::SpawnProcess(pid, [&](int fd) {
            fs::path path = argv0_path;
            path.remove_filename();
            path /= fs::PathFromString(new_exe_name);
            return std::vector<std::string>{fs::PathToString(path), "-ipcfd", strprintf("%i", fd)};
        });
    }
    int waitSpawned(int pid) override { return mp::WaitProcess(pid); }
    bool checkSpawned(int argc, char* argv[], int& fd) override
    {
        // If this process was not started with a single -ipcfd argument, it is
        // not a process spawned by the spawn() call above, so return false and
        // do not try to serve requests.
        if (argc != 3 || strcmp(argv[1], "-ipcfd") != 0) {
            return false;
        }
        // If a single -ipcfd argument was provided, return true and get the
        // file descriptor so Protocol::serve() can be called to handle
        // requests from the parent process. The -ipcfd argument is not valid
        // in combination with other arguments because the parent process
        // should be able to control the child process through the IPC protocol
        // without passing information out of band.
        if (!ParseInt32(argv[2], &fd)) {
            throw std::runtime_error(strprintf("Invalid -ipcfd number '%s'", argv[2]));
        }
        return true;
    }
};
} // namespace

std::unique_ptr<Process> MakeProcess() { return std::make_unique<ProcessImpl>(); }
} // namespace ipc
