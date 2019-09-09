// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Freicoin developers
// Copyright (c) 2011-2021 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#ifndef FREICOIN_SCRIPT_INTERPRETER_H
#define FREICOIN_SCRIPT_INTERPRETER_H

#include "script_error.h"
#include "primitives/transaction.h"

#include <vector>
#include <stdint.h>
#include <string>

class CPubKey;
class CScript;
class CTransaction;
class uint256;

/** Signature hash types/flags */
enum
{
    SIGHASH_ALL = 1,
    SIGHASH_NONE = 2,
    SIGHASH_SINGLE = 3,
    SIGHASH_ANYONECANPAY = 0x80,
    // Only set within unit tests ported over from bitcoin and
    // retained, this flag (which exceeds a byte and therefore cannot
    // be set within a serialized signature) indicates that the
    // lock_height field of CTransaction is not to be serialized
    // during signature checks, thereby preventing the invalidation of
    // bitcoin signatures contained within the unit test transaction.
    SIGHASH_NO_LOCK_HEIGHT = 0x100,
};

/** Script verification flags */
enum
{
    SCRIPT_VERIFY_NONE      = 0,

    // Evaluate P2SH subscripts (softfork safe, BIP16).
    SCRIPT_VERIFY_P2SH      = (1U << 0),

    // Passing a non-strict-DER signature or one with undefined hashtype to a checksig operation causes script failure.
    // Evaluating a pubkey that is not (0x04 + 64 bytes) or (0x02 or 0x03 + 32 bytes) by checksig causes script failure.
    // (softfork safe, but not used or intended as a consensus rule).
    SCRIPT_VERIFY_STRICTENC = (1U << 1),

    // Passing a non-strict-DER signature to a checksig operation causes script failure (softfork safe, BIP62 rule 1)
    SCRIPT_VERIFY_DERSIG    = (1U << 2),

    // Passing a non-strict-DER signature or one with S > order/2 to a checksig operation causes script failure
    // (softfork safe, BIP62 rule 5).
    SCRIPT_VERIFY_LOW_S     = (1U << 3),

    // verify dummy stack item consumed by CHECKMULTISIG is of zero-length (softfork safe, BIP62 rule 7).
    SCRIPT_VERIFY_NULLDUMMY = (1U << 4),

    // Using a non-push operator in the scriptSig causes script failure (softfork safe, BIP62 rule 2).
    SCRIPT_VERIFY_SIGPUSHONLY = (1U << 5),

    // Require minimal encodings for all push operations (OP_0... OP_16, OP_1NEGATE where possible, direct
    // pushes up to 75 bytes, OP_PUSHDATA up to 255 bytes, OP_PUSHDATA2 for anything larger). Evaluating
    // any other push causes the script to fail (BIP62 rule 3).
    // In addition, whenever a stack element is interpreted as a number, it must be of minimal length (BIP62 rule 4).
    // (softfork safe)
    SCRIPT_VERIFY_MINIMALDATA = (1U << 6),

    // Discourage use of NOPs reserved for upgrades (NOP1-10)
    //
    // Provided so that nodes can avoid accepting or mining transactions
    // containing executed NOP's whose meaning may change after a soft-fork,
    // thus rendering the script invalid; with this flag set executing
    // discouraged NOPs fails the script. This verification flag will never be
    // a mandatory flag applied to scripts in a block. NOPs that are not
    // executed, e.g.  within an unexecuted IF ENDIF block, are *not* rejected.
    //
    // Also discourage use of undefined opcodes after protocol cleanup fork activation
    //
    // If the protocol-cleanup fork is activated, undefined opcodes
    // have "return true" semantics, meaning that encountering such an
    // opcode results in the immediate SUCCESSFUL(!) termination of
    // script execution. Before activation they will be given less
    // dangerous semantics, but until then they are treated as
    // discouraged as well, even though they aren't 'NOP' opcodes as
    // the name implies.
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS  = (1U << 7),

    // Require that only a single stack element remains after evaluation. This changes the success criterion from
    // "At least one stack element must remain, and when interpreted as a boolean, it must be true" to
    // "Exactly one stack element must remain, and when interpreted as a boolean, it must be true".
    // (softfork safe, BIP62 rule 6)
    // Note: CLEANSTACK should never be used without P2SH or WITNESS.
    SCRIPT_VERIFY_CLEANSTACK = (1U << 8),

    // Verify CHECKLOCKTIMEVERIFY
    //
    // See BIP65 for details.
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9),

    // support CHECKSEQUENCEVERIFY opcode
    //
    // See BIP112 for details
    SCRIPT_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10),

    // Support segregated witness
    //
    SCRIPT_VERIFY_WITNESS = (1U << 11),

    // Making v1-v16 witness program non-standard
    //
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM = (1U << 12),

    // Segwit script only: Require the argument of OP_IF/NOTIF to be exactly 0x01 or empty vector
    //
    SCRIPT_VERIFY_MINIMALIF = (1U << 13),

    // Signature(s) must be empty vector if an CHECK(MULTI)SIG operation failed
    //
    SCRIPT_VERIFY_NULLFAIL = (1U << 14),

    // Requires the presence of a bitfield specifying which keys are
    // skipped during signature validation of a CHECKMULTISIG, using the
    // extra data push that opcode consumes (softfork safe, replaces BIP62
    // rule 7, and is not compatible with NULLDUMMY). Originally coded as
    // REQUIRE_VALID_SIGS in a softfork deployed on v12.1, the script
    // verification codes for that soft fork have now been split into
    // NULLFAIL which requires that failing signatures be empty, and
    // MULTISIG_HINT which allows matching keys to signatures prior to
    // signature verification.
    //
    // CHECKMULTISIG and CHECKMULTISIGVERIFY present a significant
    // challenge to preventing failed signature checks in that the
    // original data format did not indicate which public keys were
    // matched with which signatures, other than the ordering. For a
    // k-of-n multisig, there are n-choose-(n-k) possibilities. For
    // example, a 2-of-3 multisig would have three public keys matched
    // with two signatures, resulting in three possible assignments of
    // pubkeys to signatures. In the original implementation this is done
    // by attempting to validate a signature, starting with the first
    // public key and the first signature, and then moving to the next
    // pubkey if validation fails. It is not known in advance to the
    // validator which attempts will fail.
    //
    // Thankfully, however, a bug in the original implementation causes an
    // extra, unused item to be removed from stack after validation. Since
    // this value is given no previous consensus meaning, we use it as a
    // bitfield to indicate which pubkeys to skip. (Note that NULLDUMMY
    // would require this field to be zero, which is incompatible with
    // MULTISIG_HINT when any keys must be skipped. NULLDUMMY is retained
    // only for the purpose of compatibility with unit tests inherited
    // from the bitcoin code base.)
    //
    // Enforcing MULTISIG_HINT and NULLFAIL are necessary precursor steps
    // to performing batch validation, since in a batch validation regime
    // individual pubkey-signature combinations would not be checked for
    // validity.
    //
    // Like bitcoin's SCRIPT_VERIFY_NULLDUMMY, this also serves as a
    // malleability fix since the bitmask value is provided by the
    // witness.
    SCRIPT_VERIFY_MULTISIG_HINT = (1U << 15),

    // Public keys in segregated witness scripts must be compressed
    //
    SCRIPT_VERIFY_WITNESS_PUBKEYTYPE = (1U << 16),

    // Set if we are relaxing some of the overly restrictive protocol
    // rules as part of the "protocol cleanup" fork. See commet in
    // main.h for further description. This flag is a bit unlike the
    // other script verification flags, but it is the easiest way to
    // pass this parameter around the script validation code.
    SCRIPT_VERIFY_PROTOCOL_CLEANUP = (1U << 29),

    // If set, do not serialize CTransaction::lock_height in SignatureHash
    //
    // This exists entirely as a shim to keep valuable bitcoin unit
    // tests working within this codebase. Unit tests containing a
    // bitcoin transaction have to be rewritten to add the lock_height
    // field in order to deserialize, but passing this flag to script
    // verification ensures that the lock heights are not serialized
    // during signature verification, and therefore do not invalidate
    // the original bitcoin signatures.
    SCRIPT_VERIFY_LOCK_HEIGHT_NOT_UNDER_SIGNATURE = (1U << 30),
};

bool CheckSignatureEncoding(const std::vector<unsigned char> &vchSig, unsigned int flags, ScriptError* serror);

struct PrecomputedTransactionData
{
    uint256 hashPrevouts, hashSequence, hashOutputs;

    PrecomputedTransactionData(const CTransaction& tx);
};

enum SigVersion
{
    SIGVERSION_BASE = 0,
    SIGVERSION_WITNESS_V0 = 1,
};

uint256 SignatureHash(const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType, const CAmount& amount, int64_t refheight, SigVersion sigversion, const PrecomputedTransactionData* cache = NULL);

class BaseSignatureChecker
{
public:
    virtual bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
    {
        return false;
    }

    virtual bool CheckLockTime(const CScriptNum& nLockTime) const
    {
         return false;
    }

    virtual bool CheckSequence(const CScriptNum& nSequence) const
    {
         return false;
    }

    virtual ~BaseSignatureChecker() {}
};

enum {
    TXSIGCHECK_NONE = 0,
    TXSIGCHECK_NO_LOCK_HEIGHT = (1 << 0),
};

class TransactionSignatureChecker : public BaseSignatureChecker
{
private:
    const CTransaction* txTo;
    unsigned int nIn;
    const CAmount amount;
    const int64_t refheight;
    const PrecomputedTransactionData* txdata;
    bool no_lock_height;

protected:
    virtual bool VerifySignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const;

public:
    TransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int64_t refheightIn, int flags = TXSIGCHECK_NONE) : txTo(txToIn), nIn(nInIn), amount(amountIn), refheight(refheightIn), txdata(NULL), no_lock_height(flags & TXSIGCHECK_NO_LOCK_HEIGHT) {}
    TransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int64_t refheightIn, const PrecomputedTransactionData& txdataIn, int flags = TXSIGCHECK_NONE) : txTo(txToIn), nIn(nInIn), amount(amountIn), refheight(refheightIn), txdata(&txdataIn), no_lock_height(flags & TXSIGCHECK_NO_LOCK_HEIGHT) {}
    bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const;
    bool CheckLockTime(const CScriptNum& nLockTime) const;
    bool CheckSequence(const CScriptNum& nSequence) const;
};

class MutableTransactionSignatureChecker : public TransactionSignatureChecker
{
private:
    const CTransaction txTo;

public:
    MutableTransactionSignatureChecker(const CMutableTransaction* txToIn, unsigned int nInIn, const CAmount& amount, int64_t refheight, int flags = TXSIGCHECK_NONE) : TransactionSignatureChecker(&txTo, nInIn, amount, refheight, flags), txTo(*txToIn) {}
};

bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* error = NULL);
bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* serror = NULL);

#endif // FREICOIN_SCRIPT_INTERPRETER_H
