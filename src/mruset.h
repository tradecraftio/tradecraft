// Copyright (c) 2012 The Bitcoin developers
// Copyright (c) 2011-2019 The Freicoin developers
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

#ifndef FREICOIN_MRUSET_H
#define FREICOIN_MRUSET_H

#include <deque>
#include <set>
#include <utility>

/** STL-like set container that only keeps the most recent N elements. */
template <typename T>
class mruset
{
public:
    typedef T key_type;
    typedef T value_type;
    typedef typename std::set<T>::iterator iterator;
    typedef typename std::set<T>::const_iterator const_iterator;
    typedef typename std::set<T>::size_type size_type;

protected:
    std::set<T> set;
    std::deque<T> queue;
    size_type nMaxSize;

public:
    mruset(size_type nMaxSizeIn = 0) { nMaxSize = nMaxSizeIn; }
    iterator begin() const { return set.begin(); }
    iterator end() const { return set.end(); }
    size_type size() const { return set.size(); }
    bool empty() const { return set.empty(); }
    iterator find(const key_type& k) const { return set.find(k); }
    size_type count(const key_type& k) const { return set.count(k); }
    void clear()
    {
        set.clear();
        queue.clear();
    }
    bool inline friend operator==(const mruset<T>& a, const mruset<T>& b) { return a.set == b.set; }
    bool inline friend operator==(const mruset<T>& a, const std::set<T>& b) { return a.set == b; }
    bool inline friend operator<(const mruset<T>& a, const mruset<T>& b) { return a.set < b.set; }
    std::pair<iterator, bool> insert(const key_type& x)
    {
        std::pair<iterator, bool> ret = set.insert(x);
        if (ret.second) {
            if (nMaxSize && queue.size() == nMaxSize) {
                set.erase(queue.front());
                queue.pop_front();
            }
            queue.push_back(x);
        }
        return ret;
    }
    size_type max_size() const { return nMaxSize; }
    size_type max_size(size_type s)
    {
        if (s)
            while (queue.size() > s) {
                set.erase(queue.front());
                queue.pop_front();
            }
        nMaxSize = s;
        return nMaxSize;
    }
};

#endif // FREICOIN_MRUSET_H
