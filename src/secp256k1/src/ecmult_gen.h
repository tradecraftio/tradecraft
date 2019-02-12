/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Copyright (c) 2018-2019 The Freicoin developers                    *
 *                                                                    *
 * This program is free software: you can redistribute it and/or      *
 * modify it under the conjunctive terms of BOTH version 3 of the GNU *
 * Affero General Public License as published by the Free Software    *
 * Foundation AND the MIT/X11 software license.                       *
 *                                                                    *
 * This program is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 * Affero General Public License and the MIT/X11 software license for *
 * more details.                                                      *
 *                                                                    *
 * You should have received a copy of both licenses along with this   *
 * program.  If not, see <https://www.gnu.org/licenses/> and          *
 * <http://www.opensource.org/licenses/mit-license.php>               *
 **********************************************************************/

#ifndef _SECP256K1_ECMULT_GEN_
#define _SECP256K1_ECMULT_GEN_

#include "scalar.h"
#include "group.h"

static void secp256k1_ecmult_gen_start(void);
static void secp256k1_ecmult_gen_stop(void);

/** Multiply with the generator: R = a*G */
static void secp256k1_ecmult_gen(secp256k1_gej_t *r, const secp256k1_scalar_t *a);

#endif
