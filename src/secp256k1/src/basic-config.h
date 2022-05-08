/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Copyright (c) 2018-2022 The Freicoin Developers                    *
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

#ifndef SECP256K1_BASIC_CONFIG_H
#define SECP256K1_BASIC_CONFIG_H

#ifdef USE_BASIC_CONFIG

#undef USE_ASM_X86_64
#undef USE_ECMULT_STATIC_PRECOMPUTATION
#undef USE_EXTERNAL_ASM
#undef USE_EXTERNAL_DEFAULT_CALLBACKS
#undef USE_FIELD_INV_BUILTIN
#undef USE_FIELD_INV_NUM
#undef USE_NUM_GMP
#undef USE_NUM_NONE
#undef USE_SCALAR_INV_BUILTIN
#undef USE_SCALAR_INV_NUM
#undef USE_FORCE_WIDEMUL_INT64
#undef USE_FORCE_WIDEMUL_INT128
#undef ECMULT_WINDOW_SIZE

#define USE_NUM_NONE 1
#define USE_FIELD_INV_BUILTIN 1
#define USE_SCALAR_INV_BUILTIN 1
#define USE_WIDEMUL_64 1
#define ECMULT_WINDOW_SIZE 15

#endif /* USE_BASIC_CONFIG */

#endif /* SECP256K1_BASIC_CONFIG_H */
