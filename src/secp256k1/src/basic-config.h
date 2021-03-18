/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Copyright (c) 2018-2021 The Freicoin Developers                    *
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

#ifndef _SECP256K1_BASIC_CONFIG_
#define _SECP256K1_BASIC_CONFIG_

#ifdef USE_BASIC_CONFIG

#undef USE_ASM_X86_64
#undef USE_ENDOMORPHISM
#undef USE_FIELD_10X26
#undef USE_FIELD_5X52
#undef USE_FIELD_INV_BUILTIN
#undef USE_FIELD_INV_NUM
#undef USE_NUM_GMP
#undef USE_NUM_NONE
#undef USE_SCALAR_4X64
#undef USE_SCALAR_8X32
#undef USE_SCALAR_INV_BUILTIN
#undef USE_SCALAR_INV_NUM

#define USE_NUM_NONE 1
#define USE_FIELD_INV_BUILTIN 1
#define USE_SCALAR_INV_BUILTIN 1
#define USE_FIELD_10X26 1
#define USE_SCALAR_8X32 1

#endif // USE_BASIC_CONFIG
#endif // _SECP256K1_BASIC_CONFIG_
