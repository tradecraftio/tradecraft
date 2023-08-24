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
#include <util/tokenpipe.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#ifndef WIN32

#include <errno.h>
#include <fcntl.h>
#include <optional>
#include <unistd.h>

TokenPipeEnd TokenPipe::TakeReadEnd()
{
    TokenPipeEnd res(m_fds[0]);
    m_fds[0] = -1;
    return res;
}

TokenPipeEnd TokenPipe::TakeWriteEnd()
{
    TokenPipeEnd res(m_fds[1]);
    m_fds[1] = -1;
    return res;
}

TokenPipeEnd::TokenPipeEnd(int fd) : m_fd(fd)
{
}

TokenPipeEnd::~TokenPipeEnd()
{
    Close();
}

int TokenPipeEnd::TokenWrite(uint8_t token)
{
    while (true) {
        ssize_t result = write(m_fd, &token, 1);
        if (result < 0) {
            // Failure. It's possible that the write was interrupted by a signal,
            // in that case retry.
            if (errno != EINTR) {
                return TS_ERR;
            }
        } else if (result == 0) {
            return TS_EOS;
        } else { // ==1
            return 0;
        }
    }
}

int TokenPipeEnd::TokenRead()
{
    uint8_t token;
    while (true) {
        ssize_t result = read(m_fd, &token, 1);
        if (result < 0) {
            // Failure. Check if the read was interrupted by a signal,
            // in that case retry.
            if (errno != EINTR) {
                return TS_ERR;
            }
        } else if (result == 0) {
            return TS_EOS;
        } else { // ==1
            return token;
        }
    }
    return token;
}

void TokenPipeEnd::Close()
{
    if (m_fd != -1) close(m_fd);
    m_fd = -1;
}

std::optional<TokenPipe> TokenPipe::Make()
{
    int fds[2] = {-1, -1};
#if HAVE_O_CLOEXEC && HAVE_DECL_PIPE2
    if (pipe2(fds, O_CLOEXEC) != 0) {
        return std::nullopt;
    }
#else
    if (pipe(fds) != 0) {
        return std::nullopt;
    }
#endif
    return TokenPipe(fds);
}

TokenPipe::~TokenPipe()
{
    Close();
}

void TokenPipe::Close()
{
    if (m_fds[0] != -1) close(m_fds[0]);
    if (m_fds[1] != -1) close(m_fds[1]);
    m_fds[0] = m_fds[1] = -1;
}

#endif // WIN32
