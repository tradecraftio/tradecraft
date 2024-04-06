/*****************************************************************************************************
 * Copyright (c) 2013, 2014, 2017, 2021 Pieter Wuille, Andrew Poelstra, Jonas Nick, Russell O'Connor *
 * Copyright (c) 2018-2024 The Freicoin Developers                                                   *
 *                                                                                                   *
 * This program is free software: you can redistribute it and/or modify it under the terms of        *
 * version 3 of the GNU Affero General Public License as published by the Free Software Foundation.  *
 *                                                                                                   *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without *
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    *
 * Affero General Public License for more details.                                                   *
 *                                                                                                   *
 * You should have received a copy of the GNU Affero General Public License along with this program. *
 * If not, see <https://www.gnu.org/licenses/>.                                                      *
 *****************************************************************************************************/

#ifndef SECP256K1_PRECOMPUTED_ECMULT_H
#define SECP256K1_PRECOMPUTED_ECMULT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "group.h"
#if defined(EXHAUSTIVE_TEST_ORDER)
#    if EXHAUSTIVE_TEST_ORDER == 7
#        define WINDOW_G 3
#    elif EXHAUSTIVE_TEST_ORDER == 13
#        define WINDOW_G 4
#    elif EXHAUSTIVE_TEST_ORDER == 199
#        define WINDOW_G 8
#    else
#        error No known generator for the specified exhaustive test group order.
#    endif
static secp256k1_ge_storage secp256k1_pre_g[ECMULT_TABLE_SIZE(WINDOW_G)];
static secp256k1_ge_storage secp256k1_pre_g_128[ECMULT_TABLE_SIZE(WINDOW_G)];
#else /* !defined(EXHAUSTIVE_TEST_ORDER) */
#    define WINDOW_G ECMULT_WINDOW_SIZE
extern const secp256k1_ge_storage secp256k1_pre_g[ECMULT_TABLE_SIZE(WINDOW_G)];
extern const secp256k1_ge_storage secp256k1_pre_g_128[ECMULT_TABLE_SIZE(WINDOW_G)];
#endif /* defined(EXHAUSTIVE_TEST_ORDER) */

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_PRECOMPUTED_ECMULT_H */
