// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef FREICOIN_SCRIPT_FREICOINCONSENSUS_H
#define FREICOIN_SCRIPT_FREICOINCONSENSUS_H

#include <stdint.h>

#if defined(BUILD_FREICOIN_INTERNAL) && defined(HAVE_CONFIG_H)
#include <config/freicoin-config.h>
  #if defined(_WIN32)
    #if defined(HAVE_DLLEXPORT_ATTRIBUTE)
      #define EXPORT_SYMBOL __declspec(dllexport)
    #else
      #define EXPORT_SYMBOL
    #endif
  #elif defined(HAVE_DEFAULT_VISIBILITY_ATTRIBUTE)
    #define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
  #endif
#elif defined(MSC_VER) && !defined(STATIC_LIBFREICOINCONSENSUS)
  #define EXPORT_SYMBOL __declspec(dllimport)
#endif

#ifndef EXPORT_SYMBOL
  #define EXPORT_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FREICOINCONSENSUS_API_VER 1

typedef enum freicoinconsensus_error_t
{
    freicoinconsensus_ERR_OK = 0,
    freicoinconsensus_ERR_TX_INDEX,
    freicoinconsensus_ERR_TX_SIZE_MISMATCH,
    freicoinconsensus_ERR_TX_DESERIALIZE,
    freicoinconsensus_ERR_AMOUNT_REQUIRED,
    freicoinconsensus_ERR_INVALID_FLAGS,
} freicoinconsensus_error;

/** Script verification flags */
enum
{
    freicoinconsensus_SCRIPT_FLAGS_VERIFY_NONE                = 0,
    freicoinconsensus_SCRIPT_FLAGS_VERIFY_P2SH                = (1U << 0), // evaluate P2SH (BIP16) subscripts
    freicoinconsensus_SCRIPT_FLAGS_VERIFY_DERSIG              = (1U << 2), // enforce strict DER (BIP66) compliance
    freicoinconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY           = (1U << 4), // enforce NULLDUMMY (BIP147)
    freicoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS             = (1U << 11), // enable WITNESS (BIP141)
    freicoinconsensus_SCRIPT_FLAGS_VERIFY_ALL                 = freicoinconsensus_SCRIPT_FLAGS_VERIFY_P2SH |
                                                               freicoinconsensus_SCRIPT_FLAGS_VERIFY_DERSIG |
                                                               freicoinconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY |
                                                               freicoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS
};

/// Returns 1 if the input nIn of the serialized transaction pointed to by
/// txTo correctly spends the scriptPubKey pointed to by scriptPubKey under
/// the additional constraints specified by flags.
/// If not nullptr, err will contain an error/success code for the operation
EXPORT_SYMBOL int freicoinconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                                 const unsigned char *txTo        , unsigned int txToLen,
                                                 unsigned int nIn, unsigned int flags, freicoinconsensus_error* err);

EXPORT_SYMBOL int freicoinconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount, int64_t refheight,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, freicoinconsensus_error* err);

EXPORT_SYMBOL unsigned int freicoinconsensus_version();

#ifdef __cplusplus
} // extern "C"
#endif

#undef EXPORT_SYMBOL

#endif // FREICOIN_SCRIPT_FREICOINCONSENSUS_H
