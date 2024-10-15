/*********************************************************************************
 * Copyright (c) 2013, 2014, 2015, 2021 Thomas Daede, Cory Fields, Pieter Wuille *
 * Copyright (c) 2018-2024 The Freicoin Developers                               *
 *                                                                               *
 * This program is free software: you can redistribute it and/or modify it under *
 * the terms of version 3 of the GNU Affero General Public License as published  *
 * by the Free Software Foundation.                                              *
 *                                                                               *
 * This program is distributed in the hope that it will be useful, but WITHOUT   *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS *
 * FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more *
 * details.                                                                      *
 *                                                                               *
 * You should have received a copy of the GNU Affero General Public License      *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.        *
 *********************************************************************************/

#ifndef SECP256K1_PRECOMPUTED_ECMULT_GEN_H
#define SECP256K1_PRECOMPUTED_ECMULT_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "group.h"
#include "ecmult_gen.h"
#ifdef EXHAUSTIVE_TEST_ORDER
static secp256k1_ge_storage secp256k1_ecmult_gen_prec_table[COMB_BLOCKS][COMB_POINTS];
#else
extern const secp256k1_ge_storage secp256k1_ecmult_gen_prec_table[COMB_BLOCKS][COMB_POINTS];
#endif /* defined(EXHAUSTIVE_TEST_ORDER) */

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_PRECOMPUTED_ECMULT_GEN_H */
