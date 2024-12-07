/***********************************************************************
 * Copyright (c) 2015, 2022 Andrew Poelstra, Pieter Wuille             *
 * Copyright (c) 2018-2024 The Freicoin Developers                     *
 *                                                                     *
 * This program is free software: you can redistribute it and/or       *
 * modify it under the terms of version 3 of the GNU Affero General    *
 * Public License as published by the Free Software Foundation.        *
 *                                                                     *
 * This program is distributed in the hope that it will be useful,     *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Affero General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Affero General Public    *
 * License along with this program.  If not, see                       *
 * <https://www.gnu.org/licenses/>.                                    *
 ***********************************************************************/

#ifndef SECP256K1_SCALAR_REPR_H
#define SECP256K1_SCALAR_REPR_H

#include <stdint.h>

/** A scalar modulo the group order of the secp256k1 curve. */
typedef uint32_t secp256k1_scalar;

/* A compile-time constant equal to 2^32 (modulo order). */
#define SCALAR_2P32 ((0xffffffffUL % EXHAUSTIVE_TEST_ORDER) + 1U)

/* Compute a*2^32 + b (modulo order). */
#define SCALAR_HORNER(a, b) (((uint64_t)(a) * SCALAR_2P32 + (b)) % EXHAUSTIVE_TEST_ORDER)

/* Evaluates to the provided 256-bit constant reduced modulo order. */
#define SECP256K1_SCALAR_CONST(d7, d6, d5, d4, d3, d2, d1, d0) SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER((d7), (d6)), (d5)), (d4)), (d3)), (d2)), (d1)), (d0))

#endif /* SECP256K1_SCALAR_REPR_H */
