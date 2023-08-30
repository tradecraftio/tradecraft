/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Copyright (c) 2018-2023 The Freicoin Developers                    *
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

#ifndef BITCOIN_LIBSECP256K1_CONFIG_H
#define BITCOIN_LIBSECP256K1_CONFIG_H

#undef USE_ASM_X86_64

#define ECMULT_GEN_PREC_BITS 4
#define ECMULT_WINDOW_SIZE 15

#endif // BITCOIN_LIBSECP256K1_CONFIG_H
