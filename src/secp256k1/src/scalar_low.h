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

#ifndef _SECP256K1_SCALAR_REPR_
#define _SECP256K1_SCALAR_REPR_

#include <stdint.h>

/** A scalar modulo the group order of the secp256k1 curve. */
typedef uint32_t secp256k1_scalar;

#endif
