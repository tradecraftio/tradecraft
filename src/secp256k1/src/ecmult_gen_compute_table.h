/***********************************************************************
 * Copyright (c) Pieter Wuille, Gregory Maxwell                        *
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

#ifndef SECP256K1_ECMULT_GEN_COMPUTE_TABLE_H
#define SECP256K1_ECMULT_GEN_COMPUTE_TABLE_H

#include "ecmult_gen.h"

static void secp256k1_ecmult_gen_compute_table(secp256k1_ge_storage* table, const secp256k1_ge* gen, int blocks, int teeth, int spacing);

#endif /* SECP256K1_ECMULT_GEN_COMPUTE_TABLE_H */
