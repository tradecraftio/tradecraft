/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Copyright (c) 2018-2024 The Freicoin Developers                    *
 *                                                                    *
 * This program is free software: you can redistribute it and/or      *
 * modify it under the terms of version 3 of the GNU Affero General   *
 * Public License as published by the Free Software Foundation.       *
 *                                                                    *
 * This program is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 * Affero General Public License for more details.                    *
 *                                                                    *
 * You should have received a copy of the GNU Affero General Public   *
 * License along with this program.  If not, see                      *
 * <https://www.gnu.org/licenses/>.                                   *
 **********************************************************************/

#ifndef _MINISKETCH_STATE_H_
#define _MINISKETCH_STATE_H_

#include <stdint.h>
#include <stdlib.h>

/** Abstract class for internal representation of a minisketch object. */
class Sketch
{
    uint64_t m_canary;
    const int m_implementation;
    const int m_bits;

public:
    Sketch(int implementation, int bits) : m_implementation(implementation), m_bits(bits) {}

    void Ready() { m_canary = 0x6d496e536b65LU; }
    void Check() const { if (m_canary != 0x6d496e536b65LU) abort(); }
    void UnReady() { m_canary = 1; }
    int Implementation() const { return m_implementation; }
    int Bits() const { return m_bits; }

    virtual ~Sketch() {}
    virtual size_t Syndromes() const = 0;

    virtual void Init(size_t syndromes) = 0;
    virtual void Add(uint64_t element) = 0;
    virtual void Serialize(unsigned char*) const = 0;
    virtual void Deserialize(const unsigned char*) = 0;
    virtual size_t Merge(const Sketch* other_sketch) = 0;
    virtual void SetSeed(uint64_t seed) = 0;

    virtual int Decode(int max_count, uint64_t* roots) const = 0;
};

#endif
