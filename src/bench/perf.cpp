// Copyright (c) 2016 The Bitcoin Core developers
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

#include "perf.h"

#if defined(__i386__) || defined(__x86_64__)

/* These architectures support querying the cycle counter
 * from user space, no need for any syscall overhead.
 */
void perf_init(void) { }
void perf_fini(void) { }

#elif defined(__linux__)

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

static int fd = -1;
static struct perf_event_attr attr;

void perf_init(void)
{
    attr.type = PERF_TYPE_HARDWARE;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    fd = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
}

void perf_fini(void)
{
    if (fd != -1) {
        close(fd);
    }
}

uint64_t perf_cpucycles(void)
{
    uint64_t result = 0;
    if (fd == -1 || read(fd, &result, sizeof(result)) < (ssize_t)sizeof(result)) {
        return 0;
    }
    return result;
}

#else /* Unhandled platform */

void perf_init(void) { }
void perf_fini(void) { }
uint64_t perf_cpucycles(void) { return 0; }

#endif
