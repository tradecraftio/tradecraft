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

#if defined(ENABLE_FIELD_BYTES_INT_8)

#include "generic_common_impl.h"

#include "../lintrans.h"
#include "../sketch_impl.h"

#endif

#include "../sketch.h"

namespace {
#ifdef ENABLE_FIELD_INT_57
// 57 bit field
typedef RecLinTrans<uint64_t, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5> StatTable57;
typedef RecLinTrans<uint64_t, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3> DynTable57;
constexpr StatTable57 SQR_TABLE_57({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x100000000, 0x400000000, 0x1000000000, 0x4000000000, 0x10000000000, 0x40000000000, 0x100000000000, 0x400000000000, 0x1000000000000, 0x4000000000000, 0x10000000000000, 0x40000000000000, 0x100000000000000, 0x22, 0x88, 0x220, 0x880, 0x2200, 0x8800, 0x22000, 0x88000, 0x220000, 0x880000, 0x2200000, 0x8800000, 0x22000000, 0x88000000, 0x220000000, 0x880000000, 0x2200000000, 0x8800000000, 0x22000000000, 0x88000000000, 0x220000000000, 0x880000000000, 0x2200000000000, 0x8800000000000, 0x22000000000000, 0x88000000000000, 0x20000000000011, 0x80000000000044});
constexpr StatTable57 QRT_TABLE_57({0xd0c3a82c902426, 0x232aa54103915e, 0x232aa54103915c, 0x1763e291e61699c, 0x232aa541039158, 0x1f424d678bb15e, 0x1763e291e616994, 0x26fd8122f10d36, 0x232aa541039148, 0x1e0a0206002000, 0x1f424d678bb17e, 0x5d72563f39d7e, 0x1763e291e6169d4, 0x1519beb9d597df4, 0x26fd8122f10db6, 0x150c3a87c90e4aa, 0x232aa541039048, 0x15514891f6179d4, 0x1e0a0206002200, 0x14ec9ba7a94c6aa, 0x1f424d678bb57e, 0x1e0f4286382420, 0x5d72563f3957e, 0x4000080000, 0x1763e291e6179d4, 0x1ac0e804882000, 0x1519beb9d595df4, 0x1f430d6793b57e, 0x26fd8122f14db6, 0x3c68e806882000, 0x150c3a87c9064aa, 0x1484fe18b915e, 0x232aa541029048, 0x14f91eb9b595df4, 0x15514891f6379d4, 0x48f6a82380420, 0x1e0a0206042200, 0x14b1beb99595df4, 0x14ec9ba7a9cc6aa, 0x4cf2a82b00420, 0x1f424d679bb57e, 0x26aa0002000000, 0x1e0f4286182420, 0x173f1039dd17df4, 0x5d72563b3957e, 0x4aa0002000000, 0x4000880000, 0x16d31eb9b595df4, 0x1763e291f6179d4, 0x20000000000000, 0x1ac0e806882000, 0x2caa0002000000, 0x1519beb99595df4, 0, 0x1f430d6f93b57e, 0x73e90d6d93b57e, 0x26fd8132f14db6});
typedef Field<uint64_t, 57, 17, StatTable57, DynTable57, &SQR_TABLE_57, &QRT_TABLE_57> Field57;
#endif

#ifdef ENABLE_FIELD_INT_58
// 58 bit field
typedef RecLinTrans<uint64_t, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5> StatTable58;
typedef RecLinTrans<uint64_t, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3> DynTable58;
constexpr StatTable58 SQR_TABLE_58({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x100000000, 0x400000000, 0x1000000000, 0x4000000000, 0x10000000000, 0x40000000000, 0x100000000000, 0x400000000000, 0x1000000000000, 0x4000000000000, 0x10000000000000, 0x40000000000000, 0x100000000000000, 0x80001, 0x200004, 0x800010, 0x2000040, 0x8000100, 0x20000400, 0x80001000, 0x200004000, 0x800010000, 0x2000040000, 0x8000100000, 0x20000400000, 0x80001000000, 0x200004000000, 0x800010000000, 0x2000040000000, 0x8000100000000, 0x20000400000000, 0x80001000000000, 0x200004000000000, 0x10000100002, 0x40000400008, 0x100001000020, 0x400004000080, 0x1000010000200, 0x4000040000800, 0x10000100002000, 0x40000400008000, 0x100001000020000});
constexpr StatTable58 QRT_TABLE_58({0x2450096792a5c5c, 0x610014271011c, 0x610014271011e, 0x1f0cb811314ea88, 0x610014271011a, 0x8000000420, 0x1f0cb811314ea80, 0x265407ad8a20bcc, 0x610014271010a, 0x3d18be98392ebd0, 0x8000000400, 0xc29b930e407056, 0x1f0cb811314eac0, 0x1fcef001154dee8, 0x265407ad8a20b4c, 0xc69b924c61f94a, 0x610014271000a, 0x211006895845190, 0x3d18be98392e9d0, 0x54007accac09cc, 0x8000000000, 0xc08b934e107854, 0xc29b930e407856, 0x275407adc220bcc, 0x1f0cb811314fac0, 0x1f6db815164ea8a, 0x1fcef001154fee8, 0x1b2db801945e396, 0x265407ad8a24b4c, 0x21100ec95865590, 0xc69b924c61794a, 0x273507b1e530ad6, 0x610014270000a, 0x1b4cb835b34e29c, 0x211006895865190, 0x3839bf20d47e016, 0x3d18be98396e9d0, 0x3858bd34f36e01c, 0x54007acca409cc, 0, 0x8000100000, 0xc29a130e507856, 0xc08b934e307854, 0x13253921d448296, 0xc29b930e007856, 0x13c60935f6486bc, 0x275407adca20bcc, 0x3571be8c5e6c9da, 0x1f0cb811214fac0, 0x410014261011c, 0x1f6db815364ea8a, 0x13a50921d1486b6, 0x1fcef001554fee8, 0x64001249245a5c, 0x1b2db801145e396, 0x8610014670200a, 0x265407ac8a24b4c, 0x1a5cbfbdeb0f30c});
typedef Field<uint64_t, 58, 524289, StatTable58, DynTable58, &SQR_TABLE_58, &QRT_TABLE_58> Field58;
#endif

#ifdef ENABLE_FIELD_INT_59
// 59 bit field
typedef RecLinTrans<uint64_t, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5> StatTable59;
typedef RecLinTrans<uint64_t, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3> DynTable59;
constexpr StatTable59 SQR_TABLE_59({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x100000000, 0x400000000, 0x1000000000, 0x4000000000, 0x10000000000, 0x40000000000, 0x100000000000, 0x400000000000, 0x1000000000000, 0x4000000000000, 0x10000000000000, 0x40000000000000, 0x100000000000000, 0x400000000000000, 0x12a, 0x4a8, 0x12a0, 0x4a80, 0x12a00, 0x4a800, 0x12a000, 0x4a8000, 0x12a0000, 0x4a80000, 0x12a00000, 0x4a800000, 0x12a000000, 0x4a8000000, 0x12a0000000, 0x4a80000000, 0x12a00000000, 0x4a800000000, 0x12a000000000, 0x4a8000000000, 0x12a0000000000, 0x4a80000000000, 0x12a00000000000, 0x4a800000000000, 0x12a000000000000, 0x4a8000000000000, 0x2a000000000012a, 0x28000000000043d, 0x200000000001061});
constexpr StatTable59 QRT_TABLE_59({0x38d905ab028567a, 0x789fa6ed3b44d72, 0x789fa6ed3b44d70, 0x74ec857e93d828c, 0x789fa6ed3b44d74, 0x116b3c1203c96, 0x74ec857e93d8284, 0xc25ebc3871e280, 0x789fa6ed3b44d64, 0x47a37c3d910b6, 0x116b3c1203cb6, 0xc7322d7a8f48de, 0x74ec857e93d82c4, 0xb509a0ea52e496, 0xc25ebc3871e200, 0x74fdee4681d3e0c, 0x789fa6ed3b44c64, 0x7ffbbd080b2f09a, 0x47a37c3d912b6, 0xd5c937bae506c8, 0x116b3c12038b6, 0xb173c76987625e, 0xc7322d7a8f40de, 0x7591ff36b3a682c, 0x74ec857e93d92c4, 0x72b253bfbfc90c4, 0xb509a0ea52c496, 0x79f2e7b10e6d452, 0xc25ebc3871a200, 0x78c86e951086aac, 0x74fdee4681dbe0c, 0x78c96eb514c602c, 0x789fa6ed3b54c64, 0xc34818b95658e8, 0x7ffbbd080b0f09a, 0x7399f563b1980f2, 0x47a37c3dd12b6, 0xa29e0e28c58880, 0xd5c937baed06c8, 0x788ac23520ac82c, 0x116b3c13038b6, 0xa2c857e83d92b6, 0xb173c769a7625e, 0x608da990122e48, 0xc7322d7acf40de, 0xa3a89269eebefe, 0x7591ff36bba682c, 0xa25ebc2871a200, 0x74ec857e83d92c4, 0x11f62e419f1cfe, 0x72b253bf9fc90c4, 0x7425ebc2871a272, 0xb509a0ee52c496, 0x4ed8555979c8de, 0x79f2e7b18e6d452, 0x6c3580d5915d4d2, 0xc25ebc2871a200, 0, 0x78c86e971086aac});
typedef Field<uint64_t, 59, 149, StatTable59, DynTable59, &SQR_TABLE_59, &QRT_TABLE_59> Field59;
#endif

#ifdef ENABLE_FIELD_INT_60
// 60 bit field
typedef RecLinTrans<uint64_t, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6> StatTable60;
typedef RecLinTrans<uint64_t, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4> DynTable60;
constexpr StatTable60 SQR_TABLE_60({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x100000000, 0x400000000, 0x1000000000, 0x4000000000, 0x10000000000, 0x40000000000, 0x100000000000, 0x400000000000, 0x1000000000000, 0x4000000000000, 0x10000000000000, 0x40000000000000, 0x100000000000000, 0x400000000000000, 0x3, 0xc, 0x30, 0xc0, 0x300, 0xc00, 0x3000, 0xc000, 0x30000, 0xc0000, 0x300000, 0xc00000, 0x3000000, 0xc000000, 0x30000000, 0xc0000000, 0x300000000, 0xc00000000, 0x3000000000, 0xc000000000, 0x30000000000, 0xc0000000000, 0x300000000000, 0xc00000000000, 0x3000000000000, 0xc000000000000, 0x30000000000000, 0xc0000000000000, 0x300000000000000, 0xc00000000000000});
constexpr StatTable60 QRT_TABLE_60({0x6983c00fe00104a, 0x804570322e054e6, 0x804570322e054e4, 0x15673387e0a4e4, 0x804570322e054e0, 0x100010110, 0x15673387e0a4ec, 0x920d01f34442a70, 0x804570322e054f0, 0x7a8dc0f2e4058f0, 0x100010130, 0x120c01f140462f0, 0x15673387e0a4ac, 0x7bdbb2ca9a4fe5c, 0x920d01f34442af0, 0xe9c6b039ce0c4ac, 0x804570322e055f0, 0xfac8b080ca20c00, 0x7a8dc0f2e405af0, 0x7a8dc4b2e4a59f0, 0x100010530, 0x10000100000, 0x120c01f14046af0, 0x131a02d91c5db6c, 0x15673387e0b4ac, 0x15623387d0b4ac, 0x7bdbb2ca9a4de5c, 0x7ffbbbca0a8ee5c, 0x920d01f34446af0, 0x800000020000000, 0xe9c6b039ce044ac, 0x81130302500f000, 0x804570322e155f0, 0x935b72eb3a48e9c, 0xfac8b080ca00c00, 0x120c016140563c0, 0x7a8dc0f2e445af0, 0x7bcbb3ca8a4ee5c, 0x7a8dc4b2e4259f0, 0xc4000a0300, 0x100110530, 0x11623285c1b19c, 0x10000300000, 0x420890090c3000, 0x120c01f14446af0, 0x68d7b33b9e0b4ac, 0x131a02d9145db6c, 0xe8ccb1e18a56fc0, 0x15673386e0b4ac, 0x7aadc8f2e485af0, 0x15623385d0b4ac, 0x4a0990093c3000, 0x7bdbb2cada4de5c, 0xf9d6b3389e0b4ac, 0x7ffbbbca8a8ee5c, 0xdf6ba38cec84ac, 0x920d01f24446af0, 0x520d01f24446af0, 0x800000000000000, 0});
typedef Field<uint64_t, 60, 3, StatTable60, DynTable60, &SQR_TABLE_60, &QRT_TABLE_60> Field60;
#endif

#ifdef ENABLE_FIELD_INT_61
// 61 bit field
typedef RecLinTrans<uint64_t, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5> StatTable61;
typedef RecLinTrans<uint64_t, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3> DynTable61;
constexpr StatTable61 SQR_TABLE_61({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x100000000, 0x400000000, 0x1000000000, 0x4000000000, 0x10000000000, 0x40000000000, 0x100000000000, 0x400000000000, 0x1000000000000, 0x4000000000000, 0x10000000000000, 0x40000000000000, 0x100000000000000, 0x400000000000000, 0x1000000000000000, 0x4e, 0x138, 0x4e0, 0x1380, 0x4e00, 0x13800, 0x4e000, 0x138000, 0x4e0000, 0x1380000, 0x4e00000, 0x13800000, 0x4e000000, 0x138000000, 0x4e0000000, 0x1380000000, 0x4e00000000, 0x13800000000, 0x4e000000000, 0x138000000000, 0x4e0000000000, 0x1380000000000, 0x4e00000000000, 0x13800000000000, 0x4e000000000000, 0x138000000000000, 0x4e0000000000000, 0x1380000000000000, 0xe0000000000004e, 0x180000000000011f});
constexpr StatTable61 QRT_TABLE_61({0x171d34fcdac955d0, 0x12cfc8c049e1c96, 0x12cfc8c049e1c94, 0x71d34fcdac955c2, 0x12cfc8c049e1c90, 0x631c871de564852, 0x71d34fcdac955ca, 0x129fa6407f27300, 0x12cfc8c049e1c80, 0x7094f6fdd0a3b12, 0x631c871de564872, 0xdb28cee59c8256a, 0x71d34fcdac9558a, 0xc8a0be15a915472, 0x129fa6407f27380, 0x12dfcb4058e0b80, 0x12cfc8c049e1d80, 0x117d7f04ad0118, 0x7094f6fdd0a3912, 0x621b576dbe35b6a, 0x631c871de564c72, 0x13c808a013a1ee0, 0xdb28cee59c82d6a, 0x113d79842a0272, 0x71d34fcdac9458a, 0x719776b580b6a98, 0xc8a0be15a917472, 0x6633498d6db760a, 0x129fa6407f23380, 0xbd4ae9e8c3e7560, 0x12dfcb4058e8b80, 0x8000000a, 0x12cfc8c049f1d80, 0x634ce9add3b26ea, 0x117d7f04af0118, 0xda3f19c5d66258a, 0x7094f6fdd0e3912, 0xb87427e85e71560, 0x621b576dbeb5b6a, 0xc8b0b085b8c4e0a, 0x631c871de464c72, 0x1538fc8649458a, 0x13c808a011a1ee0, 0xcddbca6d1cfe360, 0xdb28cee59882d6a, 0xae80f550d1ffff2, 0x113d7984aa0272, 0xda7770f5f195912, 0x71d34fcdbc9458a, 0x137c8a049a1ee0, 0x719776b5a0b6a98, 0xded39a9d236ba78, 0xc8a0be15e917472, 0x6732488ca7ce0a, 0x6633498dedb760a, 0xc0406d0527cb80a, 0x129fa6417f23380, 0x3d4ae9eac3e756a, 0xbd4ae9eac3e7560, 0, 0x12dfcb4458e8b80});
typedef Field<uint64_t, 61, 39, StatTable61, DynTable61, &SQR_TABLE_61, &QRT_TABLE_61> Field61;
#endif

#ifdef ENABLE_FIELD_INT_62
// 62 bit field
typedef RecLinTrans<uint64_t, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5> StatTable62;
typedef RecLinTrans<uint64_t, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3> DynTable62;
constexpr StatTable62 SQR_TABLE_62({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x100000000, 0x400000000, 0x1000000000, 0x4000000000, 0x10000000000, 0x40000000000, 0x100000000000, 0x400000000000, 0x1000000000000, 0x4000000000000, 0x10000000000000, 0x40000000000000, 0x100000000000000, 0x400000000000000, 0x1000000000000000, 0x20000001, 0x80000004, 0x200000010, 0x800000040, 0x2000000100, 0x8000000400, 0x20000001000, 0x80000004000, 0x200000010000, 0x800000040000, 0x2000000100000, 0x8000000400000, 0x20000001000000, 0x80000004000000, 0x200000010000000, 0x800000040000000, 0x2000000100000000, 0x440000002, 0x1100000008, 0x4400000020, 0x11000000080, 0x44000000200, 0x110000000800, 0x440000002000, 0x1100000008000, 0x4400000020000, 0x11000000080000, 0x44000000200000, 0x110000000800000, 0x440000002000000, 0x1100000008000000});
constexpr StatTable62 QRT_TABLE_62({0x30268b6fba455d2c, 0x200000006, 0x200000004, 0x3d67cb6c1fe66c76, 0x200000000, 0x3fc4f1901abfa400, 0x3d67cb6c1fe66c7e, 0x35e79b6c0a66bcbe, 0x200000010, 0x1e9372bc57a9941e, 0x3fc4f1901abfa420, 0x21ec9d424957a5b0, 0x3d67cb6c1fe66c3e, 0x1cb35a6e52f5fb0e, 0x35e79b6c0a66bc3e, 0x215481024c13a730, 0x200000110, 0x1c324a6c52f75b08, 0x1e9372bc57a9961e, 0x3764a9d00f676820, 0x3fc4f1901abfa020, 0x355481020e132730, 0x21ec9d424957adb0, 0x3c43c32c0f34301e, 0x3d67cb6c1fe67c3e, 0x1496122c45259728, 0x1cb35a6e52f5db0e, 0x15e418405b72ec20, 0x35e79b6c0a66fc3e, 0x30268b6e3a445c38, 0x215481024c132730, 0x100010114, 0x200010110, 0, 0x1c324a6c52f55b08, 0x215581044d133776, 0x1e9372bc57ad961e, 0x2155810e4d133766, 0x3764a9d00f6f6820, 0x2157833c4d12323e, 0x3fc4f1901aafa020, 0x1c324a4252f55b58, 0x355481020e332730, 0x28332fc0509d41e, 0x21ec9d424917adb0, 0x215783be4d12332e, 0x3c43c32c0fb4301e, 0x2157822c4d06363e, 0x3d67cb6c1ee67c3e, 0x23f6b9d2484afb78, 0x1496122c47259728, 0x14b8184047648a80, 0x1cb35a6e56f5db0e, 0x3fe4f1901aefa820, 0x15e418405372ec20, 0x3d5fd72c1be276be, 0x35e79b6c1a66fc3e, 0x14b038d24774cf10, 0x30268b6e1a445c38, 0x1d17022e43a7172e, 0x215481020c132730, 0x2157022e4d07372e});
typedef Field<uint64_t, 62, 536870913, StatTable62, DynTable62, &SQR_TABLE_62, &QRT_TABLE_62> Field62;
#endif

#ifdef ENABLE_FIELD_INT_63
// 63 bit field
typedef RecLinTrans<uint64_t, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5> StatTable63;
typedef RecLinTrans<uint64_t, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3> DynTable63;
constexpr StatTable63 SQR_TABLE_63({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x100000000, 0x400000000, 0x1000000000, 0x4000000000, 0x10000000000, 0x40000000000, 0x100000000000, 0x400000000000, 0x1000000000000, 0x4000000000000, 0x10000000000000, 0x40000000000000, 0x100000000000000, 0x400000000000000, 0x1000000000000000, 0x4000000000000000, 0x6, 0x18, 0x60, 0x180, 0x600, 0x1800, 0x6000, 0x18000, 0x60000, 0x180000, 0x600000, 0x1800000, 0x6000000, 0x18000000, 0x60000000, 0x180000000, 0x600000000, 0x1800000000, 0x6000000000, 0x18000000000, 0x60000000000, 0x180000000000, 0x600000000000, 0x1800000000000, 0x6000000000000, 0x18000000000000, 0x60000000000000, 0x180000000000000, 0x600000000000000, 0x1800000000000000, 0x6000000000000000});
constexpr StatTable63 QRT_TABLE_63({0, 0x100010114, 0x100010116, 0x1001701051372, 0x100010112, 0x1000040220, 0x100170105137a, 0x5107703453bba, 0x100010102, 0x101130117155a, 0x1000040200, 0x40000200800, 0x100170105133a, 0x103151a137276d8, 0x5107703453b3a, 0x134e65fc7c222be0, 0x100010002, 0x100030103115a, 0x101130117175a, 0x106052d103f4de2, 0x1000040600, 0x15122707691d3a, 0x40000200000, 0x4530770bc57b3a, 0x100170105033a, 0x103011a131256d8, 0x103151a137256d8, 0x176f29eb55c7a8da, 0x5107703457b3a, 0x130b158b7767d0da, 0x134e65fc7c22abe0, 0x7bcaf59d2f62d3e2, 0x100000002, 0x1001401041260, 0x100030101115a, 0x5107e03443ab8, 0x101130113175a, 0x1043701251b3a, 0x106052d10374de2, 0x134e657d7c232be2, 0x1000140600, 0x106073d103b4be2, 0x15122707491d3a, 0x4438600ac07800, 0x40000600000, 0x176a199c5682d3e0, 0x4530770b457b3a, 0x7bca759c2f62d3e0, 0x100170005033a, 0x6116d02572de2, 0x103011a111256d8, 0x1346656d7c372de2, 0x103151a177256d8, 0x643c600aa07800, 0x176f29eb5dc7a8da, 0x7b4b758b2f67d0da, 0x5107713457b3a, 0x104570776b457b3a, 0x130b158b5767d0da, 0x734e65fc3c22abe0, 0x134e65fc3c22abe0, 0x4000000000000000, 0x7bcaf59daf62d3e2});
typedef Field<uint64_t, 63, 3, StatTable63, DynTable63, &SQR_TABLE_63, &QRT_TABLE_63> Field63;
#endif

#ifdef ENABLE_FIELD_INT_64
// 64 bit field
typedef RecLinTrans<uint64_t, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5> StatTable64;
typedef RecLinTrans<uint64_t, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4> DynTable64;
constexpr StatTable64 SQR_TABLE_64({0x1, 0x4, 0x10, 0x40, 0x100, 0x400, 0x1000, 0x4000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000, 0x4000000, 0x10000000, 0x40000000, 0x100000000, 0x400000000, 0x1000000000, 0x4000000000, 0x10000000000, 0x40000000000, 0x100000000000, 0x400000000000, 0x1000000000000, 0x4000000000000, 0x10000000000000, 0x40000000000000, 0x100000000000000, 0x400000000000000, 0x1000000000000000, 0x4000000000000000, 0x1b, 0x6c, 0x1b0, 0x6c0, 0x1b00, 0x6c00, 0x1b000, 0x6c000, 0x1b0000, 0x6c0000, 0x1b00000, 0x6c00000, 0x1b000000, 0x6c000000, 0x1b0000000, 0x6c0000000, 0x1b00000000, 0x6c00000000, 0x1b000000000, 0x6c000000000, 0x1b0000000000, 0x6c0000000000, 0x1b00000000000, 0x6c00000000000, 0x1b000000000000, 0x6c000000000000, 0x1b0000000000000, 0x6c0000000000000, 0x1b00000000000000, 0x6c00000000000000, 0xb00000000000001b, 0xc00000000000005a});
constexpr StatTable64 QRT_TABLE_64({0x19c9369f278adc02, 0x84b2b22ab2383ee4, 0x84b2b22ab2383ee6, 0x9d7b84b495b3e3f6, 0x84b2b22ab2383ee2, 0x37c470b49213f790, 0x9d7b84b495b3e3fe, 0x1000a0105137c, 0x84b2b22ab2383ef2, 0x368e964a8edce1fc, 0x37c470b49213f7b0, 0x19c9368e278fdf4c, 0x9d7b84b495b3e3be, 0x2e4da23cbc7d4570, 0x1000a010513fc, 0x84f35772bac24232, 0x84b2b22ab2383ff2, 0x37c570ba9314e4fc, 0x368e964a8edce3fc, 0xb377c390213cdb0e, 0x37c470b49213f3b0, 0x85ed5a3aa99c24f2, 0x19c9368e278fd74c, 0xaabff0000780000e, 0x9d7b84b495b3f3be, 0x84b6b3dab03038f2, 0x2e4da23cbc7d6570, 0x511ea03494ffc, 0x1000a010553fc, 0xae0c0220343c6c0e, 0x84f35772bac2c232, 0x800000008000000e, 0x84b2b22ab2393ff2, 0xb376c29c202bc97e, 0x37c570ba9316e4fc, 0x9c3062488879e6ce, 0x368e964a8ed8e3fc, 0x41e42c08e47e70, 0xb377c3902134db0e, 0x85b9b108a60f56ce, 0x37c470b49203f3b0, 0x19dd3b6e21f3cb4c, 0x85ed5a3aa9bc24f2, 0x198ddf682c428ac0, 0x19c9368e27cfd74c, 0x4b7c68431ca84b0, 0xaabff0000700000e, 0x8040655489ffefbe, 0x9d7b84b494b3f3be, 0x18c1354e32bfa74c, 0x84b6b3dab23038f2, 0xaaf613cc0f74627e, 0x2e4da23cb87d6570, 0x3248b3d6b3342a8c, 0x511ea0b494ffc, 0xb60813c00e70700e, 0x1000a110553fc, 0x1e0d022a05393ffc, 0xae0c0220143c6c0e, 0xe0c0220143c6c00, 0x84f35772fac2c232, 0xc041e55948fbfdce, 0x800000000000000e, 0});
typedef Field<uint64_t, 64, 27, StatTable64, DynTable64, &SQR_TABLE_64, &QRT_TABLE_64> Field64;
#endif
}

Sketch* ConstructGeneric8Bytes(int bits, int implementation)
{
    switch (bits) {
#ifdef ENABLE_FIELD_INT_57
    case 57: return new SketchImpl<Field57>(implementation, 57);
#endif
#ifdef ENABLE_FIELD_INT_58
    case 58: return new SketchImpl<Field58>(implementation, 58);
#endif
#ifdef ENABLE_FIELD_INT_59
    case 59: return new SketchImpl<Field59>(implementation, 59);
#endif
#ifdef ENABLE_FIELD_INT_60
    case 60: return new SketchImpl<Field60>(implementation, 60);
#endif
#ifdef ENABLE_FIELD_INT_61
    case 61: return new SketchImpl<Field61>(implementation, 61);
#endif
#ifdef ENABLE_FIELD_INT_62
    case 62: return new SketchImpl<Field62>(implementation, 62);
#endif
#ifdef ENABLE_FIELD_INT_63
    case 63: return new SketchImpl<Field63>(implementation, 63);
#endif
#ifdef ENABLE_FIELD_INT_64
    case 64: return new SketchImpl<Field64>(implementation, 64);
#endif
    default: return nullptr;
    }
}
