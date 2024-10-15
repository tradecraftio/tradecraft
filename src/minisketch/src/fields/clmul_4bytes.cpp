/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Copyright (c) 2018-2024 The Freicoin Developers                    *
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

/* This file was substantially auto-generated by doc/gen_params.sage. */
#include "../fielddefines.h"

#if defined(ENABLE_FIELD_BYTES_INT_4)

#include "clmul_common_impl.h"

#include "../int_utils.h"
#include "../lintrans.h"
#include "../sketch_impl.h"

#endif

#include "../sketch.h"

namespace {
#ifdef ENABLE_FIELD_INT_25
// 25 bit field
typedef RecLinTrans<uint32_t, 5, 5, 5, 5, 5> StatTable25;
constexpr StatTable25 SQR_TABLE_25({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x12, 0x48, 0x120, 0x480, 0x1200, 0x4800, 0x12000, 0x48000, 0x120000, 0x480000, 0x1200000, 0x800012});
constexpr StatTable25 SQR2_TABLE_25({0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x48, 0x480, 0x4800, 0x48000, 0x480000, 0x800012, 0x104, 0x1040, 0x10400, 0x104000, 0x1040000, 0x400048, 0x492, 0x4920, 0x49200, 0x492000, 0x920012, 0x1200104});
constexpr StatTable25 SQR4_TABLE_25({0x1, 0x10000, 0x480, 0x800012, 0x104000, 0x4920, 0x1200104, 0x1001000, 0x48048, 0x481040, 0x410448, 0x492492, 0x930002, 0x580, 0x1800012, 0x14c000, 0x5960, 0x160014c, 0x1493000, 0x58058, 0x5814c0, 0xc14c5a, 0x596596, 0x1974922, 0x1249684});
constexpr StatTable25 SQR8_TABLE_25({0x1, 0x5960, 0x1411448, 0x1860922, 0x1d814d2, 0x1cdede8, 0x1e15e16, 0x1b79686, 0xfdf116, 0x1efe4c8, 0x1b839a8, 0x10ced66, 0xae05ce, 0x1459400, 0xa29fa6, 0x85e4d2, 0x7eecee, 0x183a96, 0x1eb2fa8, 0xede876, 0xf6e440, 0x1f7140a, 0xd07d7c, 0x10e4ea2, 0x1222a54});
constexpr StatTable25 QRT_TABLE_25({0, 0x482110, 0x482112, 0x1b3c3e6, 0x482116, 0x4960ae, 0x1b3c3ee, 0x4088, 0x482106, 0x58a726, 0x49608e, 0x5ce52e, 0x1b3c3ae, 0x2006, 0x4008, 0x1c1a8, 0x482006, 0x1e96488, 0x58a526, 0x400000, 0x49648e, 0x1800006, 0x5ced2e, 0xb3d3a8, 0x1b3d3ae});
typedef Field<uint32_t, 25, 9, StatTable25, &SQR_TABLE_25, &SQR2_TABLE_25, &SQR4_TABLE_25, &SQR8_TABLE_25, &QRT_TABLE_25, &QRT_TABLE_25, IdTrans, &ID_TRANS, &ID_TRANS> Field25;
typedef FieldTri<uint32_t, 25, 3, RecLinTrans<uint32_t, 5, 5, 5, 5, 5>, &SQR_TABLE_25, &SQR2_TABLE_25, &SQR4_TABLE_25, &SQR8_TABLE_25, &QRT_TABLE_25, &QRT_TABLE_25, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri25;
#endif

#ifdef ENABLE_FIELD_INT_26
// 26 bit field
typedef RecLinTrans<uint32_t, 6, 5, 5, 5, 5> StatTable26;
constexpr StatTable26 SQR_TABLE_26({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x1b, 0x6c, 0x1b0, 0x6c0, 0x1b00, 0x6c00, 0x1b000, 0x6c000, 0x1b0000, 0x6c0000, 0x1b00000, 0x2c0001b, 0x300005a});
constexpr StatTable26 SQR2_TABLE_26({0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x6c, 0x6c0, 0x6c00, 0x6c000, 0x6c0000, 0x2c0001b, 0x145, 0x1450, 0x14500, 0x145000, 0x1450000, 0x500077, 0x100076b, 0x76dc, 0x76dc0, 0x76dc00, 0x36dc01b, 0x2dc011f, 0x1c01105});
constexpr StatTable26 SQR4_TABLE_26({0x1, 0x10000, 0x6c0, 0x2c0001b, 0x145000, 0x76dc, 0x2dc011f, 0x1101100, 0x106ac6c, 0x6ad515, 0x1145127, 0x121b6dc, 0x2da1d0f, 0x10007c1, 0x3c7c01b, 0x128290, 0x29062e0, 0x2ee8d68, 0x167abcd, 0x3cabbce, 0x3c7a862, 0x6b83ce, 0x3cf5620, 0x229b787, 0x38a6b0f, 0x3071ade});
constexpr StatTable26 SQR8_TABLE_26({0x1, 0x29062e0, 0x2b2942d, 0x34ab63, 0x3bddebb, 0x7b1823, 0x58b9ae, 0x391720e, 0x1385e18, 0x3891746, 0x13069c5, 0x2dfd089, 0x12a35ff, 0x3e534f, 0x172c6a2, 0x55338f, 0x3887137, 0x3f45b03, 0x164a695, 0x2c7e7ef, 0x29c907d, 0x636c85, 0x3db4007, 0x97e7ff, 0x3cbfe55, 0x31c0d96});
constexpr StatTable26 QRT_TABLE_26({0x217b530, 0x2ae82a8, 0x2ae82aa, 0x2001046, 0x2ae82ae, 0x2de032e, 0x200104e, 0x70c10c, 0x2ae82be, 0x20151f2, 0x2de030e, 0xbc1400, 0x200100e, 0x178570, 0x70c18c, 0x2ae4232, 0x2ae83be, 0x211d742, 0x20153f2, 0x21f54f2, 0x2de070e, 0x5e0700, 0xbc1c00, 0x3abb97e, 0x200000e, 0});
typedef Field<uint32_t, 26, 27, StatTable26, &SQR_TABLE_26, &SQR2_TABLE_26, &SQR4_TABLE_26, &SQR8_TABLE_26, &QRT_TABLE_26, &QRT_TABLE_26, IdTrans, &ID_TRANS, &ID_TRANS> Field26;
#endif

#ifdef ENABLE_FIELD_INT_27
// 27 bit field
typedef RecLinTrans<uint32_t, 6, 6, 5, 5, 5> StatTable27;
constexpr StatTable27 SQR_TABLE_27({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x4e, 0x138, 0x4e0, 0x1380, 0x4e00, 0x13800, 0x4e000, 0x138000, 0x4e0000, 0x1380000, 0x4e00000, 0x380004e, 0x600011f});
constexpr StatTable27 SQR2_TABLE_27({0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x4e, 0x4e0, 0x4e00, 0x4e000, 0x4e0000, 0x4e00000, 0x600011f, 0x1054, 0x10540, 0x105400, 0x1054000, 0x54004e, 0x54004e0, 0x4004f76, 0x4f658, 0x4f6580, 0x4f65800, 0x765811f, 0x658101a, 0x5810004});
constexpr StatTable27 SQR4_TABLE_27({0x1, 0x10000, 0x4e0, 0x4e00000, 0x105400, 0x4004f76, 0x765811f, 0x1001110, 0x114e04e, 0x4abe54, 0x6551445, 0x45e212e, 0x13ccbdc, 0x3d805ef, 0x5e10100, 0x114b0e0, 0xe4bf22, 0x721c505, 0x51b3ba8, 0x3bf04d5, 0x4dabba0, 0x3b0aa45, 0x24a80cb, 0xc3d4b0, 0x4b34626, 0x6372e18, 0x6028c1b});
constexpr StatTable27 SQR8_TABLE_27({0x1, 0xe4bf22, 0x430cb3c, 0x73b7225, 0x6526539, 0x3c278e3, 0x4724a6e, 0x48b39b4, 0x1dbf7de, 0x106508, 0x3564785, 0x33ae33f, 0x61d6685, 0x6adaca3, 0x2786b6f, 0x4e76784, 0x869f42, 0x466b048, 0x415e00e, 0x46c3c9a, 0x73ffd91, 0x49002e0, 0x3734fed, 0x3c04a43, 0x191d3ee, 0xe828b9, 0xfab68c});
constexpr StatTable27 QRT_TABLE_27({0x6bf0530, 0x2be4496, 0x2be4494, 0x2bf0522, 0x2be4490, 0x1896cca, 0x2bf052a, 0x408a, 0x2be4480, 0x368ae72, 0x1896cea, 0x18d2ee0, 0x2bf056a, 0x1c76d6a, 0x400a, 0x336e9f8, 0x2be4580, 0x36baf12, 0x368ac72, 0x430360, 0x18968ea, 0x34a6b80, 0x18d26e0, 0xbf1560, 0x2bf156a, 0, 0x1c74d6a});
typedef Field<uint32_t, 27, 39, StatTable27, &SQR_TABLE_27, &SQR2_TABLE_27, &SQR4_TABLE_27, &SQR8_TABLE_27, &QRT_TABLE_27, &QRT_TABLE_27, IdTrans, &ID_TRANS, &ID_TRANS> Field27;
#endif

#ifdef ENABLE_FIELD_INT_28
// 28 bit field
typedef RecLinTrans<uint32_t, 6, 6, 6, 5, 5> StatTableTRI28;
constexpr StatTableTRI28 SQR_TABLE_TRI28({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x3, 0xc, 0x30, 0xc0, 0x300, 0xc00, 0x3000, 0xc000, 0x30000, 0xc0000, 0x300000, 0xc00000, 0x3000000, 0xc000000});
constexpr StatTableTRI28 SQR2_TABLE_TRI28({0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x3, 0x30, 0x300, 0x3000, 0x30000, 0x300000, 0x3000000, 0x5, 0x50, 0x500, 0x5000, 0x50000, 0x500000, 0x5000000, 0xf, 0xf0, 0xf00, 0xf000, 0xf0000, 0xf00000, 0xf000000});
constexpr StatTableTRI28 SQR4_TABLE_TRI28({0x1, 0x10000, 0x30, 0x300000, 0x500, 0x5000000, 0xf000, 0x11, 0x110000, 0x330, 0x3300000, 0x5500, 0x500000f, 0xff000, 0x101, 0x1010000, 0x3030, 0x300005, 0x50500, 0x50000f0, 0xf0f000, 0x1111, 0x1110003, 0x33330, 0x3300055, 0x555500, 0x5000fff, 0xffff000});
constexpr StatTableTRI28 SQR8_TABLE_TRI28({0x1, 0x3030, 0x5000500, 0xf0e111, 0x3210000, 0x6300faa, 0x40ef10e, 0x501, 0xf0c030, 0x5110630, 0x395b444, 0x621010e, 0x6010f9b, 0x13bc4cb, 0x110001, 0x3303065, 0xff50f, 0xf0e120, 0x3243530, 0x330fabb, 0x5ec232c, 0x511050e, 0x3c1c064, 0x2ec60a, 0x3954175, 0x7c5c43d, 0x20acba, 0x943bc43});
constexpr StatTableTRI28 QRT_TABLE_TRI28({0x121d57a, 0x40216, 0x40214, 0x8112578, 0x40210, 0x10110, 0x8112570, 0x12597ec, 0x40200, 0x6983e00, 0x10130, 0x972b99c, 0x8112530, 0x8002000, 0x125976c, 0x815a76c, 0x40300, 0x936b29c, 0x6983c00, 0x97bb8ac, 0x10530, 0x9103000, 0x972b19c, 0xf6384ac, 0x8113530, 0x4113530, 0x8000000, 0});
typedef FieldTri<uint32_t, 28, 1, StatTableTRI28, &SQR_TABLE_TRI28, &SQR2_TABLE_TRI28, &SQR4_TABLE_TRI28, &SQR8_TABLE_TRI28, &QRT_TABLE_TRI28, &QRT_TABLE_TRI28, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri28;
#endif

#ifdef ENABLE_FIELD_INT_29
// 29 bit field
typedef RecLinTrans<uint32_t, 6, 6, 6, 6, 5> StatTable29;
constexpr StatTable29 SQR_TABLE_29({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0xa, 0x28, 0xa0, 0x280, 0xa00, 0x2800, 0xa000, 0x28000, 0xa0000, 0x280000, 0xa00000, 0x2800000, 0xa000000, 0x8000005});
constexpr StatTable29 SQR2_TABLE_29({0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x10000000, 0x28, 0x280, 0x2800, 0x28000, 0x280000, 0x2800000, 0x8000005, 0x44, 0x440, 0x4400, 0x44000, 0x440000, 0x4400000, 0x400000a, 0xaa, 0xaa0, 0xaa00, 0xaa000, 0xaa0000, 0xaa00000, 0xa000011});
constexpr StatTable29 SQR4_TABLE_29({0x1, 0x10000, 0x28, 0x280000, 0x440, 0x4400000, 0xaa00, 0xa000011, 0x101000, 0x10000280, 0x2828000, 0x4444, 0x444000a, 0xaaaa0, 0xaa00101, 0x1000100, 0x1002800, 0x8002805, 0x8044005, 0x440aa, 0xaa00aa, 0xaa1010, 0x10101010, 0x10128280, 0x28282c4, 0x2c44444, 0x4444eaa, 0xeaaaaaa, 0xaaba001});
constexpr StatTable29 SQR8_TABLE_29({0x1, 0x1002800, 0x4680000, 0xae50ba, 0x2822a00, 0x14545eba, 0x110aed64, 0xc6eeaaf, 0x4ee00a0, 0x10aba290, 0x1bd6efc1, 0x8222b29, 0x1c791ebf, 0x174e85da, 0x1cc66c7f, 0x29292c4, 0x2886c20, 0xea04467, 0xc0eeb87, 0xccd4115, 0x16d5fa2e, 0x1cf8fe75, 0xe45a4e1, 0x19018b3f, 0x1d64778, 0x2e0bdf8, 0xa1bd96b, 0xff5b70e, 0x14d89770});
constexpr StatTable29 QRT_TABLE_29({0x1b8351dc, 0xb87135e, 0xb87135c, 0xda7b35e, 0xb871358, 0x621a116, 0xda7b356, 0x40200, 0xb871348, 0xc9e2620, 0x621a136, 0x478b16, 0xda7b316, 0x6762e20, 0x40280, 0x6202000, 0xb871248, 0x627a316, 0xc9e2420, 0xcd1ad36, 0x621a536, 0x760e20, 0x478316, 0xa760e20, 0xda7a316, 0x8000000, 0x6760e20, 0, 0x44280});
typedef Field<uint32_t, 29, 5, StatTable29, &SQR_TABLE_29, &SQR2_TABLE_29, &SQR4_TABLE_29, &SQR8_TABLE_29, &QRT_TABLE_29, &QRT_TABLE_29, IdTrans, &ID_TRANS, &ID_TRANS> Field29;
typedef FieldTri<uint32_t, 29, 2, RecLinTrans<uint32_t, 6, 6, 6, 6, 5>, &SQR_TABLE_29, &SQR2_TABLE_29, &SQR4_TABLE_29, &SQR8_TABLE_29, &QRT_TABLE_29, &QRT_TABLE_29, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri29;
#endif

#ifdef ENABLE_FIELD_INT_30
// 30 bit field
typedef RecLinTrans<uint32_t, 6, 6, 6, 6, 6> StatTableTRI30;
constexpr StatTableTRI30 SQR_TABLE_TRI30({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x3, 0xc, 0x30, 0xc0, 0x300, 0xc00, 0x3000, 0xc000, 0x30000, 0xc0000, 0x300000, 0xc00000, 0x3000000, 0xc000000, 0x30000000});
constexpr StatTableTRI30 SQR2_TABLE_TRI30({0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x10000000, 0xc, 0xc0, 0xc00, 0xc000, 0xc0000, 0xc00000, 0xc000000, 0x5, 0x50, 0x500, 0x5000, 0x50000, 0x500000, 0x5000000, 0x10000003, 0x3c, 0x3c0, 0x3c00, 0x3c000, 0x3c0000, 0x3c00000, 0x3c000000});
constexpr StatTableTRI30 SQR4_TABLE_TRI30({0x1, 0x10000, 0xc, 0xc0000, 0x50, 0x500000, 0x3c0, 0x3c00000, 0x1100, 0x11000000, 0xcc00, 0xc000005, 0x55000, 0x1000003f, 0x3fc000, 0x101, 0x1010000, 0xc0c, 0xc0c0000, 0x5050, 0x10500003, 0x3c3c0, 0x3c00011, 0x111100, 0x110000cc, 0xcccc00, 0xc000555, 0x5555000, 0x10003fff, 0x3fffc000});
constexpr StatTableTRI30 SQR8_TABLE_TRI30({0x1, 0x1010000, 0xc000c, 0xc0c5050, 0x390, 0x13900012, 0x12c012c0, 0x121ddddd, 0x54100, 0x1003f33, 0xc3f0d04, 0x9555558, 0xd379000, 0x105d3fa2, 0x1d615e9e, 0x1101, 0x100100cc, 0xc0ccc09, 0x5590505, 0x3a9390, 0x3913fec, 0x13fedfcd, 0x121ddd8c, 0x11544103, 0x2cc3cff, 0x3e24c45, 0x9558bc8, 0x3a7958b, 0x1e98b158, 0x29d629e9});
constexpr StatTableTRI30 QRT_TABLE_TRI30({0x2159df4a, 0x109134a, 0x1091348, 0x10114, 0x109134c, 0x3a203420, 0x1011c, 0x20004080, 0x109135c, 0x2005439c, 0x3a203400, 0x100400, 0x1015c, 0x3eb21930, 0x20004000, 0x20504c00, 0x109125c, 0x3b2b276c, 0x2005419c, 0x210450c0, 0x3a203000, 0x3e93186c, 0x100c00, 0x3aa23530, 0x1115c, 0x6b3286c, 0x3eb23930, 0xeb23930, 0x20000000, 0});
typedef FieldTri<uint32_t, 30, 1, StatTableTRI30, &SQR_TABLE_TRI30, &SQR2_TABLE_TRI30, &SQR4_TABLE_TRI30, &SQR8_TABLE_TRI30, &QRT_TABLE_TRI30, &QRT_TABLE_TRI30, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri30;
#endif

#ifdef ENABLE_FIELD_INT_31
// 31 bit field
typedef RecLinTrans<uint32_t, 6, 5, 5, 5, 5, 5> StatTable31;
constexpr StatTable31 SQR_TABLE_31({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x12, 0x48, 0x120, 0x480, 0x1200, 0x4800, 0x12000, 0x48000, 0x120000, 0x480000, 0x1200000, 0x4800000, 0x12000000, 0x48000000, 0x20000012});
constexpr StatTable31 SQR2_TABLE_31({0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x10000000, 0x12, 0x120, 0x1200, 0x12000, 0x120000, 0x1200000, 0x12000000, 0x20000012, 0x104, 0x1040, 0x10400, 0x104000, 0x1040000, 0x10400000, 0x4000012, 0x40000120, 0x1248, 0x12480, 0x124800, 0x1248000, 0x12480000, 0x24800012, 0x48000104});
constexpr StatTable31 SQR4_TABLE_31({0x1, 0x10000, 0x12, 0x120000, 0x104, 0x1040000, 0x1248, 0x12480000, 0x10010, 0x100012, 0x120120, 0x1200104, 0x1041040, 0x10401248, 0x12492480, 0x24810002, 0x112, 0x1120000, 0x1304, 0x13040000, 0x11648, 0x16480012, 0x134810, 0x48100116, 0x1121120, 0x11201304, 0x13053040, 0x3041165a, 0x16596492, 0x64934922, 0x49248016});
constexpr StatTable31 SQR8_TABLE_31({0x1, 0x112, 0x10104, 0x1131648, 0x10002, 0x1120224, 0x106021a, 0x146e3f86, 0x16, 0x174c, 0x161658, 0x175b1130, 0x16002c, 0x174c2e98, 0x16742dfc, 0x3f877966, 0x114, 0x10768, 0x1151050, 0x66b75b2, 0x1140228, 0x76a0ec2, 0x127a33da, 0x79648102, 0x1738, 0x1665f0, 0x172f64e0, 0x73cc668c, 0x17382e70, 0x65dccaac, 0x4abf956e});
constexpr StatTable31 QRT_TABLE_31({0, 0x10110, 0x10112, 0x15076e, 0x10116, 0x117130e, 0x150766, 0x4743fa0, 0x10106, 0x1121008, 0x117132e, 0x176b248e, 0x150726, 0x172a2c88, 0x4743f20, 0x7eb81e86, 0x10006, 0x20008, 0x1121208, 0x56b2c8e, 0x117172e, 0x133f1bae, 0x176b2c8e, 0x7f2a0c8e, 0x151726, 0x10000000, 0x172a0c88, 0x60000006, 0x4747f20, 0x3eb89e80, 0x7eb89e86});
typedef Field<uint32_t, 31, 9, StatTable31, &SQR_TABLE_31, &SQR2_TABLE_31, &SQR4_TABLE_31, &SQR8_TABLE_31, &QRT_TABLE_31, &QRT_TABLE_31, IdTrans, &ID_TRANS, &ID_TRANS> Field31;
typedef FieldTri<uint32_t, 31, 3, RecLinTrans<uint32_t, 6, 5, 5, 5, 5, 5>, &SQR_TABLE_31, &SQR2_TABLE_31, &SQR4_TABLE_31, &SQR8_TABLE_31, &QRT_TABLE_31, &QRT_TABLE_31, IdTrans, &ID_TRANS, &ID_TRANS> FieldTri31;
#endif

#ifdef ENABLE_FIELD_INT_32
// 32 bit field
typedef RecLinTrans<uint32_t, 6, 6, 5, 5, 5, 5> StatTable32;
constexpr StatTable32 SQR_TABLE_32({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x8d, 0x234, 0x8d0, 0x2340, 0x8d00, 0x23400, 0x8d000, 0x234000, 0x8d0000, 0x2340000, 0x8d00000, 0x23400000, 0x8d000000, 0x3400011a, 0xd0000468, 0x40001037});
constexpr StatTable32 SQR2_TABLE_32({0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x10000000, 0x8d, 0x8d0, 0x8d00, 0x8d000, 0x8d0000, 0x8d00000, 0x8d000000, 0xd0000468, 0x4051, 0x40510, 0x405100, 0x4051000, 0x40510000, 0x5100234, 0x51002340, 0x100236b9, 0x236b1d, 0x236b1d0, 0x236b1d00, 0x36b1d11a, 0x6b1d1037, 0xb1d1005e, 0x1d10001f, 0xd100017d});
constexpr StatTable32 SQR4_TABLE_32({0x1, 0x10000, 0x8d, 0x8d0000, 0x4051, 0x40510000, 0x236b1d, 0x6b1d1037, 0x10001101, 0x1109d000, 0xd00859e5, 0x59881468, 0x144737e8, 0x37e2c4e3, 0xc4f9a67a, 0xa61d8c55, 0x8c010001, 0x41dc8d, 0xdc8d23cd, 0x23a60c51, 0xc41630e, 0x63087fcd, 0x7ffe7368, 0x735580f6, 0x80cd8e29, 0x8e6fe311, 0xe350f32b, 0xf35edc90, 0xdced0bd6, 0xbbd3eb1, 0x3eb4a621, 0xa63f6bc4});
constexpr StatTable32 SQR8_TABLE_32({0x1, 0x8c010001, 0x6b9010bb, 0x7faf6b, 0xc4da8d37, 0xc10ab646, 0x445f546c, 0xe389129e, 0xd8aa2d3e, 0x85249468, 0xd599253f, 0x458976f9, 0xc9c86411, 0xccc2f34b, 0xa79e37dc, 0x9068e3c4, 0x3a30447f, 0x674c3398, 0x94f38a7, 0x402d3532, 0x116fffc7, 0x1c6b5ba2, 0xcd6a32e4, 0x49067a77, 0xa7f6a61e, 0x3cc3746, 0xeebe962e, 0x599276e1, 0x7b5fa4d9, 0x2aa3ce1, 0x990f8767, 0x1c3b66cb});
constexpr StatTable32 QRT_TABLE_32({0x54fd1264, 0xc26fcd64, 0xc26fcd66, 0x238a7462, 0xc26fcd62, 0x973bccaa, 0x238a746a, 0x77766712, 0xc26fcd72, 0xc1bdd556, 0x973bcc8a, 0x572a094c, 0x238a742a, 0xb693be84, 0x77766792, 0x9555c03e, 0xc26fcc72, 0x568419f8, 0xc1bdd756, 0x96c3d2ca, 0x973bc88a, 0x54861fdc, 0x572a014c, 0xb79badc4, 0x238a642a, 0xb9b99fe0, 0xb6939e84, 0xc519fa86, 0x77762792, 0, 0x9555403e, 0x377627ba});
typedef Field<uint32_t, 32, 141, StatTable32, &SQR_TABLE_32, &SQR2_TABLE_32, &SQR4_TABLE_32, &SQR8_TABLE_32, &QRT_TABLE_32, &QRT_TABLE_32, IdTrans, &ID_TRANS, &ID_TRANS> Field32;
#endif
}

Sketch* ConstructClMul4Bytes(int bits, int implementation) {
    switch (bits) {
#ifdef ENABLE_FIELD_INT_25
    case 25: return new SketchImpl<Field25>(implementation, 25);
#endif
#ifdef ENABLE_FIELD_INT_26
    case 26: return new SketchImpl<Field26>(implementation, 26);
#endif
#ifdef ENABLE_FIELD_INT_27
    case 27: return new SketchImpl<Field27>(implementation, 27);
#endif
#ifdef ENABLE_FIELD_INT_29
    case 29: return new SketchImpl<Field29>(implementation, 29);
#endif
#ifdef ENABLE_FIELD_INT_31
    case 31: return new SketchImpl<Field31>(implementation, 31);
#endif
#ifdef ENABLE_FIELD_INT_32
    case 32: return new SketchImpl<Field32>(implementation, 32);
#endif
    }
    return nullptr;
}

Sketch* ConstructClMulTri4Bytes(int bits, int implementation) {
    switch (bits) {
#ifdef ENABLE_FIELD_INT_25
    case 25: return new SketchImpl<FieldTri25>(implementation, 25);
#endif
#ifdef ENABLE_FIELD_INT_28
    case 28: return new SketchImpl<FieldTri28>(implementation, 28);
#endif
#ifdef ENABLE_FIELD_INT_29
    case 29: return new SketchImpl<FieldTri29>(implementation, 29);
#endif
#ifdef ENABLE_FIELD_INT_30
    case 30: return new SketchImpl<FieldTri30>(implementation, 30);
#endif
#ifdef ENABLE_FIELD_INT_31
    case 31: return new SketchImpl<FieldTri31>(implementation, 31);
#endif
    }
    return nullptr;
}
