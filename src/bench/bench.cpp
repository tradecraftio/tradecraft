// Copyright (c) 2015 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
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

#include "bench.h"

#include <iostream>
#include <iomanip>
#include <sys/time.h>

using namespace benchmark;

std::map<std::string, BenchFunction> BenchRunner::benchmarks;

static double gettimedouble(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec * 0.000001 + tv.tv_sec;
}

BenchRunner::BenchRunner(std::string name, BenchFunction func)
{
    benchmarks.insert(std::make_pair(name, func));
}

void
BenchRunner::RunAll(double elapsedTimeForOne)
{
    std::cout << "#Benchmark" << "," << "count" << "," << "min" << "," << "max" << "," << "average" << "\n";

    for (std::map<std::string,BenchFunction>::iterator it = benchmarks.begin();
         it != benchmarks.end(); ++it) {

        State state(it->first, elapsedTimeForOne);
        BenchFunction& func = it->second;
        func(state);
    }
}

bool State::KeepRunning()
{
    if (count & countMask) {
      ++count;
      return true;
    }
    double now;
    if (count == 0) {
        lastTime = beginTime = now = gettimedouble();
    }
    else {
        now = gettimedouble();
        double elapsed = now - lastTime;
        double elapsedOne = elapsed * countMaskInv;
        if (elapsedOne < minTime) minTime = elapsedOne;
        if (elapsedOne > maxTime) maxTime = elapsedOne;
        if (elapsed*128 < maxElapsed) {
          // If the execution was much too fast (1/128th of maxElapsed), increase the count mask by 8x and restart timing.
          // The restart avoids including the overhead of this code in the measurement.
          countMask = ((countMask<<3)|7) & ((1LL<<60)-1);
          countMaskInv = 1./(countMask+1);
          count = 0;
          minTime = std::numeric_limits<double>::max();
          maxTime = std::numeric_limits<double>::min();
          return true;
        }
        if (elapsed*16 < maxElapsed) {
          uint64_t newCountMask = ((countMask<<1)|1) & ((1LL<<60)-1);
          if ((count & newCountMask)==0) {
              countMask = newCountMask;
              countMaskInv = 1./(countMask+1);
          }
        }
    }
    lastTime = now;
    ++count;

    if (now - beginTime < maxElapsed) return true; // Keep going

    --count;

    // Output results
    double average = (now-beginTime)/count;
    std::cout << std::fixed << std::setprecision(15) << name << "," << count << "," << minTime << "," << maxTime << "," << average << "\n";

    return false;
}
