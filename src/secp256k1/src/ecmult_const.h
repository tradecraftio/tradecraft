/**********************************************************************
 * Copyright (c) 2015 Andrew Poelstra                                 *
 * Copyright (c) 2018-2021 The Freicoin Developers                    *
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

#ifndef SECP256K1_ECMULT_CONST_H
#define SECP256K1_ECMULT_CONST_H

#include "scalar.h"
#include "group.h"

/* Here `bits` should be set to the maximum bitlength of the _absolute value_ of `q`, plus
 * one because we internally sometimes add 2 to the number during the WNAF conversion. */
static void secp256k1_ecmult_const(secp256k1_gej *r, const secp256k1_ge *a, const secp256k1_scalar *q, int bits);

#endif /* SECP256K1_ECMULT_CONST_H */
