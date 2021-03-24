/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
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

#ifndef _SECP256K1_NUM_IMPL_H_
#define _SECP256K1_NUM_IMPL_H_

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#include "num.h"

#if defined(USE_NUM_GMP)
#include "num_gmp_impl.h"
#elif defined(USE_NUM_NONE)
/* Nothing. */
#else
#error "Please select num implementation"
#endif

#endif
