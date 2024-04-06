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

#ifndef SECP256K1_ECMULT_COMPUTE_TABLE_H
#define SECP256K1_ECMULT_COMPUTE_TABLE_H

/* Construct table of all odd multiples of gen in range 1..(2**(window_g-1)-1). */
static void secp256k1_ecmult_compute_table(secp256k1_ge_storage* table, int window_g, const secp256k1_gej* gen);

/* Like secp256k1_ecmult_compute_table, but one for both gen and gen*2^128. */
static void secp256k1_ecmult_compute_two_tables(secp256k1_ge_storage* table, secp256k1_ge_storage* table_128, int window_g, const secp256k1_ge* gen);

#endif /* SECP256K1_ECMULT_COMPUTE_TABLE_H */
