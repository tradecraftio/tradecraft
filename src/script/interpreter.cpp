// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2011-2021 The Freicoin Developers
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

#include "interpreter.h"

#include "primitives/transaction.h"
#include "consensus/merkle.h"
#include "consensus/merkleproof.h"
#include "crypto/ripemd160.h"
#include "crypto/sha1.h"
#include "crypto/sha256.h"
#include "pubkey.h"
#include "script/script.h"
#include "streams.h"
#include "uint256.h"

#include <bitset>

typedef std::vector<unsigned char> valtype;

namespace {

inline bool set_success(ScriptError* ret)
{
    if (ret)
        *ret = SCRIPT_ERR_OK;
    return true;
}

inline bool set_error(ScriptError* ret, const ScriptError serror)
{
    if (ret)
        *ret = serror;
    return false;
}

} // namespace

bool CastToBool(const valtype& vch)
{
    for (unsigned int i = 0; i < vch.size(); i++)
    {
        if (vch[i] != 0)
        {
            // Can be negative zero
            if (i == vch.size()-1 && vch[i] == 0x80)
                return false;
            return true;
        }
    }
    return false;
}

/**
 * Script is a stack machine (like Forth) that evaluates a predicate
 * returning a bool indicating valid or not.  There are no loops.
 */
#define stacktop(i)  (stack.at(stack.size()+(i)))
#define altstacktop(i)  (altstack.at(altstack.size()+(i)))
static inline void popstack(std::vector<valtype>& stack)
{
    if (stack.empty())
        throw std::runtime_error("popstack(): stack empty");
    stack.pop_back();
}

bool static IsCompressedOrUncompressedPubKey(const valtype &vchPubKey) {
    if (vchPubKey.size() < 33) {
        //  Non-canonical public key: too short
        return false;
    }
    if (vchPubKey[0] == 0x04) {
        if (vchPubKey.size() != 65) {
            //  Non-canonical public key: invalid length for uncompressed key
            return false;
        }
    } else if (vchPubKey[0] == 0x02 || vchPubKey[0] == 0x03) {
        if (vchPubKey.size() != 33) {
            //  Non-canonical public key: invalid length for compressed key
            return false;
        }
    } else {
        //  Non-canonical public key: neither compressed nor uncompressed
        return false;
    }
    return true;
}

bool static IsCompressedPubKey(const valtype &vchPubKey) {
    if (vchPubKey.size() != 33) {
        //  Non-canonical public key: invalid length for compressed key
        return false;
    }
    if (vchPubKey[0] != 0x02 && vchPubKey[0] != 0x03) {
        //  Non-canonical public key: invalid prefix for compressed key
        return false;
    }
    return true;
}

/**
 * A canonical signature exists of: <30> <total len> <02> <len R> <R> <02> <len S> <S> <hashtype>
 * Where R and S are not negative (their first byte has its highest bit not set), and not
 * excessively padded (do not start with a 0 byte, unless an otherwise negative number follows,
 * in which case a single 0 byte is necessary and even required).
 * 
 * See https://bitcointalk.org/index.php?topic=8392.msg127623#msg127623
 *
 * This function is consensus-critical since BIP66.
 */
bool static IsValidSignatureEncoding(const std::vector<unsigned char> &sig) {
    // Format: 0x30 [total-length] 0x02 [R-length] [R] 0x02 [S-length] [S] [sighash]
    // * total-length: 1-byte length descriptor of everything that follows,
    //   excluding the sighash byte.
    // * R-length: 1-byte length descriptor of the R value that follows.
    // * R: arbitrary-length big-endian encoded R value. It must use the shortest
    //   possible encoding for a positive integers (which means no null bytes at
    //   the start, except a single one when the next byte has its highest bit set).
    // * S-length: 1-byte length descriptor of the S value that follows.
    // * S: arbitrary-length big-endian encoded S value. The same rules apply.
    // * sighash: 1-byte value indicating what data is hashed (not part of the DER
    //   signature)

    // Minimum and maximum size constraints.
    if (sig.size() < 9) return false;
    if (sig.size() > 73) return false;

    // A signature is of type 0x30 (compound).
    if (sig[0] != 0x30) return false;

    // Make sure the length covers the entire signature.
    if (sig[1] != sig.size() - 3) return false;

    // Extract the length of the R element.
    unsigned int lenR = sig[3];

    // Make sure the length of the S element is still inside the signature.
    if (5 + lenR >= sig.size()) return false;

    // Extract the length of the S element.
    unsigned int lenS = sig[5 + lenR];

    // Verify that the length of the signature matches the sum of the length
    // of the elements.
    if ((size_t)(lenR + lenS + 7) != sig.size()) return false;
 
    // Check whether the R element is an integer.
    if (sig[2] != 0x02) return false;

    // Zero-length integers are not allowed for R.
    if (lenR == 0) return false;

    // Negative numbers are not allowed for R.
    if (sig[4] & 0x80) return false;

    // Null bytes at the start of R are not allowed, unless R would
    // otherwise be interpreted as a negative number.
    if (lenR > 1 && (sig[4] == 0x00) && !(sig[5] & 0x80)) return false;

    // Check whether the S element is an integer.
    if (sig[lenR + 4] != 0x02) return false;

    // Zero-length integers are not allowed for S.
    if (lenS == 0) return false;

    // Negative numbers are not allowed for S.
    if (sig[lenR + 6] & 0x80) return false;

    // Null bytes at the start of S are not allowed, unless S would otherwise be
    // interpreted as a negative number.
    if (lenS > 1 && (sig[lenR + 6] == 0x00) && !(sig[lenR + 7] & 0x80)) return false;

    return true;
}

bool static IsLowDERSignature(const valtype &vchSig, ScriptError* serror) {
    if (!IsValidSignatureEncoding(vchSig)) {
        return set_error(serror, SCRIPT_ERR_SIG_DER);
    }
    std::vector<unsigned char> vchSigCopy(vchSig.begin(), vchSig.begin() + vchSig.size() - 1);
    if (!CPubKey::CheckLowS(vchSigCopy)) {
        return set_error(serror, SCRIPT_ERR_SIG_HIGH_S);
    }
    return true;
}

bool static IsDefinedHashtypeSignature(const valtype &vchSig) {
    if (vchSig.size() == 0) {
        return false;
    }
    unsigned char nHashType = vchSig[vchSig.size() - 1] & (~(SIGHASH_ANYONECANPAY));
    if (nHashType < SIGHASH_ALL || nHashType > SIGHASH_SINGLE)
        return false;

    return true;
}

bool CheckSignatureEncoding(const std::vector<unsigned char> &vchSig, unsigned int flags, ScriptError* serror) {
    // Empty signature. Not strictly DER encoded, but allowed to provide a
    // compact way to provide an invalid signature for use with CHECK(MULTI)SIG
    if (vchSig.size() == 0) {
        return true;
    }
    if ((flags & (SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_LOW_S | SCRIPT_VERIFY_STRICTENC)) != 0 && !IsValidSignatureEncoding(vchSig)) {
        return set_error(serror, SCRIPT_ERR_SIG_DER);
    } else if ((flags & SCRIPT_VERIFY_LOW_S) != 0 && !IsLowDERSignature(vchSig, serror)) {
        // serror is set
        return false;
    } else if ((flags & SCRIPT_VERIFY_STRICTENC) != 0 && !IsDefinedHashtypeSignature(vchSig)) {
        return set_error(serror, SCRIPT_ERR_SIG_HASHTYPE);
    }
    return true;
}

bool static CheckPubKeyEncoding(const valtype &vchPubKey, unsigned int flags, const SigVersion &sigversion, ScriptError* serror) {
    if ((flags & SCRIPT_VERIFY_STRICTENC) != 0 && !IsCompressedOrUncompressedPubKey(vchPubKey)) {
        return set_error(serror, SCRIPT_ERR_PUBKEYTYPE);
    }
    // Only compressed keys are accepted in segwit
    if ((flags & SCRIPT_VERIFY_WITNESS_PUBKEYTYPE) != 0 && sigversion == SIGVERSION_WITNESS_V0 && !IsCompressedPubKey(vchPubKey)) {
        return set_error(serror, SCRIPT_ERR_WITNESS_PUBKEYTYPE);
    }
    return true;
}

bool static CheckMinimalPush(const valtype& data, opcodetype opcode) {
    if (data.size() == 0) {
        // Could have used OP_0.
        return opcode == OP_0;
    } else if (data.size() == 1 && data[0] >= 1 && data[0] <= 16) {
        // Could have used OP_1 .. OP_16.
        return opcode == OP_1 + (data[0] - 1);
    } else if (data.size() == 1 && data[0] == 0x81) {
        // Could have used OP_1NEGATE.
        return opcode == OP_1NEGATE;
    } else if (data.size() <= 75) {
        // Could have used a direct push (opcode indicating number of bytes pushed + those bytes).
        return opcode == data.size();
    } else if (data.size() <= 255) {
        // Could have used OP_PUSHDATA.
        return opcode == OP_PUSHDATA1;
    } else if (data.size() <= 65535) {
        // Could have used OP_PUSHDATA2.
        return opcode == OP_PUSHDATA2;
    }
    return true;
}

bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror)
{
    static const CScriptNum bnZero(0);
    static const CScriptNum bnOne(1);
    // static const CScriptNum bnFalse(0);
    // static const CScriptNum bnTrue(1);
    static const valtype vchFalse(0);
    // static const valtype vchZero(0);
    static const valtype vchTrue(1, 1);

    // Check for activation of rule changes
    const bool protocol_cleanup = (flags & SCRIPT_VERIFY_PROTOCOL_CLEANUP) != 0;
    const bool discourage_upgradable_nops = (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS) != 0;
    const bool enforce_nullfail = (sigversion != SIGVERSION_BASE) || ((flags & SCRIPT_VERIFY_NULLFAIL) != 0);
    const bool enforce_multisig_hint = (sigversion != SIGVERSION_BASE) || ((flags & SCRIPT_VERIFY_MULTISIG_HINT) != 0);

    CScript::const_iterator pc = script.begin();
    CScript::const_iterator pend = script.end();
    CScript::const_iterator pbegincodehash = script.begin();
    opcodetype opcode;
    valtype vchPushValue;
    std::vector<bool> vfExec;
    std::vector<valtype> altstack;
    set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    if (!protocol_cleanup && (sigversion == SIGVERSION_BASE) && (script.size() > MAX_SCRIPT_SIZE))
        return set_error(serror, SCRIPT_ERR_SCRIPT_SIZE);
    int nOpCount = 0;
    bool fRequireMinimal = (sigversion != SIGVERSION_BASE) || ((flags & SCRIPT_VERIFY_MINIMALDATA) != 0);

    try
    {
        while (pc < pend)
        {
            bool fExec = !count(vfExec.begin(), vfExec.end(), false);

            //
            // Read instruction
            //
            // Note: GetOp only fails if the instruction was a
            // malformed push, or if (due to some bug) the code
            // pointer points beyond the end of the script. We
            // therefore don't relax this "bad opcode" restriction in
            // the protocol cleanup. Valid decoded but unrecognized
            // instructions will be handled later.
            if (!script.GetOp(pc, opcode, vchPushValue))
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            if (!protocol_cleanup && (sigversion == SIGVERSION_BASE) && (vchPushValue.size() > MAX_SCRIPT_ELEMENT_SIZE))
                return set_error(serror, SCRIPT_ERR_PUSH_SIZE);

            // Note how OP_RESERVED does not count towards the opcode limit.
            if (!protocol_cleanup && (sigversion == SIGVERSION_BASE) && (opcode > OP_16 && ++nOpCount > MAX_OPS_PER_SCRIPT))
                return set_error(serror, SCRIPT_ERR_OP_COUNT);

            if (!protocol_cleanup && (sigversion == SIGVERSION_BASE) && (
                opcode == OP_CAT ||
                opcode == OP_SUBSTR ||
                opcode == OP_LEFT ||
                opcode == OP_RIGHT ||
                opcode == OP_INVERT ||
                opcode == OP_AND ||
                opcode == OP_OR ||
                opcode == OP_XOR ||
                opcode == OP_2MUL ||
                opcode == OP_2DIV ||
                opcode == OP_MUL ||
                opcode == OP_DIV ||
                opcode == OP_MOD ||
                opcode == OP_LSHIFT ||
                opcode == OP_RSHIFT))
                return set_error(serror, SCRIPT_ERR_DISABLED_OPCODE); // Disabled opcodes.

            if (fExec && 0 <= opcode && opcode <= OP_PUSHDATA4) {
                if (fRequireMinimal && !CheckMinimalPush(vchPushValue, opcode)) {
                    return set_error(serror, SCRIPT_ERR_MINIMALDATA);
                }
                stack.push_back(vchPushValue);
            } else if (fExec || (OP_IF <= opcode && opcode <= OP_ENDIF))
            switch (opcode)
            {
                //
                // Push value
                //
                case OP_1NEGATE:
                case OP_1:
                case OP_2:
                case OP_3:
                case OP_4:
                case OP_5:
                case OP_6:
                case OP_7:
                case OP_8:
                case OP_9:
                case OP_10:
                case OP_11:
                case OP_12:
                case OP_13:
                case OP_14:
                case OP_15:
                case OP_16:
                {
                    // ( -- value)
                    CScriptNum bn((int)opcode - (int)(OP_1 - 1));
                    stack.push_back(bn.getvch());
                    // The result of these opcodes should always be the minimal way to push the data
                    // they push, so no need for a CheckMinimalPush here.
                }
                break;


                //
                // Control
                //
                case OP_NOP:
                    break;

                case OP_CHECKLOCKTIMEVERIFY:
                {
                    if (sigversion == SIGVERSION_BASE) {
                        // not enabled; treat as a NOP2
                        // in legacy scripts, same as NOP
                        if (discourage_upgradable_nops) {
                            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                        }
                        // in post-cleanup scripts, return true
                        if (protocol_cleanup) {
                            altstack.clear();
                            stack.clear();
                            stack.push_back(vchTrue);
                            return set_success(serror);
                        }
                        break;
                    }

                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    // Note that elsewhere numeric opcodes are limited to
                    // operands in the range -2**31+1 to 2**31-1, however it is
                    // legal for opcodes to produce results exceeding that
                    // range. This limitation is implemented by CScriptNum's
                    // default 4-byte limit.
                    //
                    // If we kept to that limit we'd have a year 2038 problem,
                    // even though the nLockTime field in transactions
                    // themselves is uint32 which only becomes meaningless
                    // after the year 2106.
                    //
                    // Thus as a special case we tell CScriptNum to accept up
                    // to 5-byte bignums, which are good until 2**39-1, well
                    // beyond the 2**32-1 limit of the nLockTime field itself.
                    const CScriptNum nLockTime(stacktop(-1), true, 5);

                    // In the rare event that the argument may be < 0 due to
                    // some arithmetic being done first, you can always use
                    // 0 MAX CHECKLOCKTIMEVERIFY.
                    if (nLockTime < 0)
                        return set_error(serror, SCRIPT_ERR_NEGATIVE_LOCKTIME);

                    // Actually compare the specified lock time with the transaction.
                    if (!checker.CheckLockTime(nLockTime))
                        return set_error(serror, SCRIPT_ERR_UNSATISFIED_LOCKTIME);

                    // Drop the argument from the stack.
                    popstack(stack);
                }
                break;

                case OP_CHECKSEQUENCEVERIFY:
                {
                    if (sigversion == SIGVERSION_BASE) {
                        // not enabled; treat as a NOP3
                        // in legacy scripts, same as NOP
                        if (discourage_upgradable_nops) {
                            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                        }
                        // in post-cleanup scripts, return true
                        if (protocol_cleanup) {
                            altstack.clear();
                            stack.clear();
                            stack.push_back(vchTrue);
                            return set_success(serror);
                        }
                        break;
                    }

                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    // nSequence, like nLockTime, is a 32-bit unsigned integer
                    // field. See the comment in CHECKLOCKTIMEVERIFY regarding
                    // 5-byte numeric operands.
                    const CScriptNum nSequence(stacktop(-1), true, 5);

                    // To provide for future soft-fork extensibility, if the
                    // operand has the disabled lock-time flag set, the current
                    // set of CHECKSEQUENCEVERIFY checks are not performed.
                    if ((stacktop(-1).size() <= 4) || ((stacktop(-1)[3] & 0x80) == 0)) {
                        // In the rare event that the argument may be < 0 due to
                        // some arithmetic being done first, you can always use
                        // 0 MAX CHECKSEQUENCEVERIFY.
                        if (nSequence < 0)
                            return set_error(serror, SCRIPT_ERR_NEGATIVE_LOCKTIME);

                        // Compare the specified sequence number with the input.
                        if (!checker.CheckSequence(nSequence))
                            return set_error(serror, SCRIPT_ERR_UNSATISFIED_LOCKTIME);
                    }

                    // Drop the argument from the stack.
                    popstack(stack);
                }
                break;

                case OP_MERKLEBRANCHVERIFY:
                {
                    if (sigversion == SIGVERSION_BASE) {
                        // not enabled; treat as a NOP4
                        // in legacy scripts, same as NOP
                        if (discourage_upgradable_nops) {
                            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                        }
                        // in post-cleanup scripts, return true
                        if (protocol_cleanup) {
                            altstack.clear();
                            stack.clear();
                            stack.push_back(vchTrue);
                            return set_success(serror);
                        }
                        break;
                    }

                    // ([...verify hashes...] proof root {2*count+prehash})
                    if (stack.size() < 3) {
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    }

                    valtype& vchCount = stacktop(-1);
                    valtype& vchRoot  = stacktop(-2);
                    valtype& vchProof = stacktop(-3);

                    // vchCount is a minimally encoded CScriptNum encoding
                    // count, the number of leaf elements, with the sign bit
                    // representing whether the leaf elements are prehashed.
                    bool prehashed = false;
                    std::size_t count = 0;
                    try {
                        // MAX_STACK_SIZE prevents count from ever being more
                        // than 32764 leaf values, which also means the first
                        // parameter can never be more than two bytes, when
                        // minimally encoded.
                        auto param = CScriptNum(vchCount, true, 2).getint();
                        prehashed = (param < 0);
                        count = abs(param);
                    } catch (scriptnum_error e) {
                        // param is more than 2 bytes or not minimally encoded
                        return set_error(serror, SCRIPT_ERR_MINIMALDATA);
                    } catch (...) {
                        // Belt and suspenders. It should not be possible for
                        // other exceptions to be thrown, but in case that
                        // assessment is wrong or ever changes, let's not mask
                        // other exceptions.
                        throw;
                    }

                    // There are count-many leaf objects passed on the stack
                    // after the first three parameters which are always present.
                    if (stack.size() < (3 + count)) {
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    }

                    // vchRoot is a standard 32-byte hash. Note that this hash
                    // is pushed as data and not minimally encoded.
                    if (vchRoot.size() != 32) {
                        return set_error(serror, SCRIPT_ERR_INVALID_HASH_LENGTH);
                    }
                    const uint256 root = uint256(vchRoot);

                    // The third argument is a MerkleProof, which we deserialize
                    // as part of the MerkleTree structure we are building to
                    // validate the entire root.
                    MerkleTree branch;
                    CDataStream proofStream(vchProof, SER_NETWORK, PROTOCOL_VERSION);
                    try {
                        proofStream >> branch.m_proof;
                    } catch (const std::bad_alloc e) {
                        throw; // Don't mask a transient out-of-memory exception
                    } catch (...) {
                        return set_error(serror, SCRIPT_ERR_INVALID_MERKLE_PROOF);
                    }
                    if (!proofStream.empty()) {
                        // Extra bytes remaining after the MerkleProof was
                        // deserialized, which could be a source of witness
                        // malleability.
                        return set_error(serror, SCRIPT_ERR_INVALID_MERKLE_PROOF);
                    }
                    if (branch.m_proof.m_path.dirty()) {
                        // Extra bits in the final byte of the packed
                        // serialization of the Merkle branch's path,
                        // which would otherwise be another source of
                        // witness malleability.
                        return set_error(serror, SCRIPT_ERR_INVALID_MERKLE_PROOF);
                    }
                    if ((!branch.m_proof.m_path.empty() || count || !branch.m_proof.m_skip.empty()) &&
                        ((count + branch.m_proof.m_skip.size()) != (branch.m_proof.m_path.size() + 1)))
                    {
                        // It is a property of any binary tree that the number
                        // of leaf nodes is precisely one more than the number
                        // of internal nodes.  This acts as an early-out check
                        // of whether this is a well-formed proof. Note that the
                        // special case of a 0-node, 0-verify, 0-skip tree is
                        // exempted from this requirement.
                        return set_error(serror, SCRIPT_ERR_INVALID_MERKLE_PROOF);
                    }

                    // The remaining _count_ items on the stack are the verify
                    // hashes, or the actual leaf values which are hashed with
                    // double-SHA256 to get the verify hashes if _prehashed_ is
                    // clear.
                    branch.m_verify.reserve(count);
                    for (int i = 0; i < (int)count; ++i) {
                        // -1 through -3 are the count+prehashed, root hash, and
                        // MerkleProof we already extracted.
                        valtype& vchLeaf = stacktop(-4 - i);
                        if (prehashed) {
                            // Require 32-byte hash values, no truncation of ending bytes.
                            if (vchLeaf.size() != 32) {
                                return set_error(serror, SCRIPT_ERR_INVALID_HASH_LENGTH);
                            }
                            branch.m_verify.emplace_back(vchLeaf);
                        } else {
                            branch.m_verify.emplace_back();
                            CHash256().Write(vchLeaf.data(), vchLeaf.size()).Finalize(branch.m_verify.back().begin());
                        }
                    }

                    // Compute Merkle root hash and compare
                    bool invalid = false;
                    uint256 result = branch.GetHash(&invalid);
                    if (invalid) {
                        return set_error(serror, SCRIPT_ERR_INVALID_MERKLE_PROOF);
                    }
                    if (result != root) {
                        return set_error(serror, SCRIPT_ERR_MERKLEBRANCHVERIFY);
                    }

                    // Drop the count, root, and proof arguments from the stack.
                    popstack(stack);
                    popstack(stack);
                    popstack(stack);
                }
                break;

                case OP_NOP1: case OP_NOP5:
                case OP_NOP6: case OP_NOP7: case OP_NOP8: case OP_NOP9: case OP_NOP10:
                {
                    // in legacy scripts, same as NOP
                    if (discourage_upgradable_nops)
                        return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                    // in post-segwit scripts, return true
                    if (protocol_cleanup || (sigversion != SIGVERSION_BASE)) {
                        altstack.clear();
                        stack.clear();
                        stack.push_back(vchTrue);
                        return set_success(serror);
                    }
                }
                break;

                case OP_IF:
                case OP_NOTIF:
                {
                    // <expression> if [statements] [else [statements]] endif
                    bool fValue = false;
                    if (fExec)
                    {
                        if (stack.size() < 1)
                            return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
                        valtype& vch = stacktop(-1);
                        fValue = CastToBool(vch);
                        if (opcode == OP_NOTIF)
                            fValue = !fValue;
                        popstack(stack);
                    }
                    vfExec.push_back(fValue);
                }
                break;

                case OP_ELSE:
                {
                    if (vfExec.empty())
                        return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
                    vfExec.back() = !vfExec.back();
                }
                break;

                case OP_ENDIF:
                {
                    if (vfExec.empty())
                        return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
                    vfExec.pop_back();
                }
                break;

                case OP_VERIFY:
                {
                    // (true -- ) or
                    // (false -- false) and return
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    bool fValue = CastToBool(stacktop(-1));
                    if (fValue)
                        popstack(stack);
                    else
                        return set_error(serror, SCRIPT_ERR_VERIFY);
                }
                break;

                case OP_RETURN:
                {
                    return set_error(serror, SCRIPT_ERR_OP_RETURN);
                }
                break;


                //
                // Stack ops
                //
                case OP_TOALTSTACK:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    altstack.push_back(stacktop(-1));
                    popstack(stack);
                }
                break;

                case OP_FROMALTSTACK:
                {
                    if (altstack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_ALTSTACK_OPERATION);
                    stack.push_back(altstacktop(-1));
                    popstack(altstack);
                }
                break;

                case OP_2DROP:
                {
                    // (x1 x2 -- )
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    popstack(stack);
                    popstack(stack);
                }
                break;

                case OP_2DUP:
                {
                    // (x1 x2 -- x1 x2 x1 x2)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-2);
                    valtype vch2 = stacktop(-1);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                }
                break;

                case OP_3DUP:
                {
                    // (x1 x2 x3 -- x1 x2 x3 x1 x2 x3)
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-3);
                    valtype vch2 = stacktop(-2);
                    valtype vch3 = stacktop(-1);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                    stack.push_back(vch3);
                }
                break;

                case OP_2OVER:
                {
                    // (x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2)
                    if (stack.size() < 4)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-4);
                    valtype vch2 = stacktop(-3);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                }
                break;

                case OP_2ROT:
                {
                    // (x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2)
                    if (stack.size() < 6)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-6);
                    valtype vch2 = stacktop(-5);
                    stack.erase(stack.end()-6, stack.end()-4);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                }
                break;

                case OP_2SWAP:
                {
                    // (x1 x2 x3 x4 -- x3 x4 x1 x2)
                    if (stack.size() < 4)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    swap(stacktop(-4), stacktop(-2));
                    swap(stacktop(-3), stacktop(-1));
                }
                break;

                case OP_IFDUP:
                {
                    // (x - 0 | x x)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-1);
                    if (CastToBool(vch))
                        stack.push_back(vch);
                }
                break;

                case OP_DEPTH:
                {
                    // -- stacksize
                    CScriptNum bn(stack.size());
                    stack.push_back(bn.getvch());
                }
                break;

                case OP_DROP:
                {
                    // (x -- )
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    popstack(stack);
                }
                break;

                case OP_DUP:
                {
                    // (x -- x x)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-1);
                    stack.push_back(vch);
                }
                break;

                case OP_NIP:
                {
                    // (x1 x2 -- x2)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    stack.erase(stack.end() - 2);
                }
                break;

                case OP_OVER:
                {
                    // (x1 x2 -- x1 x2 x1)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-2);
                    stack.push_back(vch);
                }
                break;

                case OP_ROLL:
                {
                    if (sigversion != SIGVERSION_BASE) {
                        // not enabled; treat as SUCCESS122
                        if (discourage_upgradable_nops) {
                            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                        }
                        altstack.clear();
                        stack.clear();
                        stack.push_back(vchTrue);
                        return set_success(serror);
                    }
                    // in legacy scripts, fall through
                }
                case OP_PICK: // and OP_ROLL
                {
                    // (xn ... x2 x1 x0 n - xn ... x2 x1 x0 xn)
                    // (xn ... x2 x1 x0 n - ... x2 x1 x0 xn)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    int n = CScriptNum(stacktop(-1), fRequireMinimal).getint();
                    popstack(stack);
                    if (n < 0 || n >= (int)stack.size())
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-n-1);
                    if (opcode == OP_ROLL)
                        stack.erase(stack.end()-n-1);
                    stack.push_back(vch);
                }
                break;

                case OP_ROT:
                {
                    // (x1 x2 x3 -- x2 x3 x1)
                    //  x2 x1 x3  after first swap
                    //  x2 x3 x1  after second swap
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    swap(stacktop(-3), stacktop(-2));
                    swap(stacktop(-2), stacktop(-1));
                }
                break;

                case OP_SWAP:
                {
                    // (x1 x2 -- x2 x1)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    swap(stacktop(-2), stacktop(-1));
                }
                break;

                case OP_TUCK:
                {
                    // (x1 x2 -- x2 x1 x2)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-1);
                    stack.insert(stack.end()-2, vch);
                }
                break;


                case OP_SIZE:
                {
                    // (in -- in size)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn(stacktop(-1).size());
                    stack.push_back(bn.getvch());
                }
                break;


                //
                // Bitwise logic
                //
                case OP_EQUAL:
                case OP_EQUALVERIFY:
                //case OP_NOTEQUAL: // use OP_NUMNOTEQUAL
                {
                    // (x1 x2 - bool)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype& vch1 = stacktop(-2);
                    valtype& vch2 = stacktop(-1);
                    bool fEqual = (vch1 == vch2);
                    // OP_NOTEQUAL is disabled because it would be too easy to say
                    // something like n != 1 and have some wiseguy pass in 1 with extra
                    // zero bytes after it (numerically, 0x01 == 0x0001 == 0x000001)
                    //if (opcode == OP_NOTEQUAL)
                    //    fEqual = !fEqual;
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fEqual ? vchTrue : vchFalse);
                    if (opcode == OP_EQUALVERIFY)
                    {
                        if (fEqual)
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_EQUALVERIFY);
                    }
                }
                break;


                //
                // Numeric
                //
                case OP_1ADD:
                case OP_1SUB:
                case OP_NEGATE:
                case OP_ABS:
                case OP_NOT:
                case OP_0NOTEQUAL:
                {
                    // (in -- out)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn(stacktop(-1), fRequireMinimal);
                    switch (opcode)
                    {
                    case OP_1ADD:       bn += bnOne; break;
                    case OP_1SUB:       bn -= bnOne; break;
                    case OP_NEGATE:     bn = -bn; break;
                    case OP_ABS:        if (bn < bnZero) bn = -bn; break;
                    case OP_NOT:        bn = (bn == bnZero); break;
                    case OP_0NOTEQUAL:  bn = (bn != bnZero); break;
                    default:            assert(!"invalid opcode"); break;
                    }
                    popstack(stack);
                    stack.push_back(bn.getvch());
                }
                break;

                case OP_ADD:
                case OP_SUB:
                case OP_BOOLAND:
                case OP_BOOLOR:
                case OP_NUMEQUAL:
                case OP_NUMEQUALVERIFY:
                case OP_NUMNOTEQUAL:
                case OP_LESSTHAN:
                case OP_GREATERTHAN:
                case OP_LESSTHANOREQUAL:
                case OP_GREATERTHANOREQUAL:
                case OP_MIN:
                case OP_MAX:
                {
                    // (x1 x2 -- out)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn1(stacktop(-2), fRequireMinimal);
                    CScriptNum bn2(stacktop(-1), fRequireMinimal);
                    CScriptNum bn(0);
                    switch (opcode)
                    {
                    case OP_ADD:
                        bn = bn1 + bn2;
                        break;

                    case OP_SUB:
                        bn = bn1 - bn2;
                        break;

                    case OP_BOOLAND:             bn = (bn1 != bnZero && bn2 != bnZero); break;
                    case OP_BOOLOR:              bn = (bn1 != bnZero || bn2 != bnZero); break;
                    case OP_NUMEQUAL:            bn = (bn1 == bn2); break;
                    case OP_NUMEQUALVERIFY:      bn = (bn1 == bn2); break;
                    case OP_NUMNOTEQUAL:         bn = (bn1 != bn2); break;
                    case OP_LESSTHAN:            bn = (bn1 < bn2); break;
                    case OP_GREATERTHAN:         bn = (bn1 > bn2); break;
                    case OP_LESSTHANOREQUAL:     bn = (bn1 <= bn2); break;
                    case OP_GREATERTHANOREQUAL:  bn = (bn1 >= bn2); break;
                    case OP_MIN:                 bn = (bn1 < bn2 ? bn1 : bn2); break;
                    case OP_MAX:                 bn = (bn1 > bn2 ? bn1 : bn2); break;
                    default:                     assert(!"invalid opcode"); break;
                    }
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(bn.getvch());

                    if (opcode == OP_NUMEQUALVERIFY)
                    {
                        if (CastToBool(stacktop(-1)))
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_NUMEQUALVERIFY);
                    }
                }
                break;

                case OP_WITHIN:
                {
                    // (x min max -- out)
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn1(stacktop(-3), fRequireMinimal);
                    CScriptNum bn2(stacktop(-2), fRequireMinimal);
                    CScriptNum bn3(stacktop(-1), fRequireMinimal);
                    bool fValue = (bn2 <= bn1 && bn1 < bn3);
                    popstack(stack);
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fValue ? vchTrue : vchFalse);
                }
                break;


                //
                // Crypto
                //
                case OP_SHA1:
                    // In 2019, SHA1 is utterly broken and no longer serves any
                    // purpose.  We therefore return OP_SHA1 to the pool of
                    // unallocated opcodes in future script versions.
                    if (sigversion != SIGVERSION_BASE) {
                        // Copy-paste of the unrecognized-opcode "default" handler below.
                        if (discourage_upgradable_nops) {
                            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                        }
                        altstack.clear();
                        stack.clear();
                        stack.push_back(vchTrue);
                        return set_success(serror);
                    }

                    // Otherwise we fall-though to the legacy handler with the
                    // original SHA1 semantics:

                case OP_RIPEMD160:
                case OP_SHA256:
                case OP_HASH160:
                case OP_HASH256:
                {
                    // (in -- hash)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype& vch = stacktop(-1);
                    valtype vchHash((opcode == OP_RIPEMD160 || opcode == OP_SHA1 || opcode == OP_HASH160) ? 20 : 32);
                    if (opcode == OP_RIPEMD160)
                        CRIPEMD160().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_SHA1)
                        CSHA1().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_SHA256)
                        CSHA256().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_HASH160)
                        CHash160().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_HASH256)
                        CHash256().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    popstack(stack);
                    stack.push_back(vchHash);
                }
                break;                                   

                case OP_CODESEPARATOR:
                {
                    // Hash starts after the code separator
                    pbegincodehash = pc;
                }
                break;

                case OP_CHECKSIG:
                case OP_CHECKSIGVERIFY:
                {
                    // (sig pubkey -- bool)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    valtype& vchSig    = stacktop(-2);
                    valtype& vchPubKey = stacktop(-1);

                    // Subset of script starting at the most recent codeseparator
                    CScript scriptCode(pbegincodehash, pend);

                    // Drop the signature in pre-segwit scripts but not segwit scripts
                    if (sigversion == SIGVERSION_BASE) {
                        scriptCode.FindAndDelete(CScript(vchSig));
                    }

                    if (!CheckSignatureEncoding(vchSig, flags, serror) || !CheckPubKeyEncoding(vchPubKey, flags, sigversion, serror)) {
                        //serror is set
                        return false;
                    }
                    bool fSuccess = checker.CheckSig(vchSig, vchPubKey, scriptCode, sigversion);

                    if (!fSuccess && enforce_nullfail && vchSig.size())
                        return set_error(serror, SCRIPT_ERR_NULLFAIL);

                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fSuccess ? vchTrue : vchFalse);
                    if (opcode == OP_CHECKSIGVERIFY)
                    {
                        if (fSuccess)
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_CHECKSIGVERIFY);
                    }
                }
                break;

                case OP_CHECKMULTISIG:
                case OP_CHECKMULTISIGVERIFY:
                {
                    // ([sig ...] num_of_signatures [pubkey ...] num_of_pubkeys -- bool)

                    int i = 1;
                    if ((int)stack.size() < i)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    int nKeysCount = CScriptNum(stacktop(-i), fRequireMinimal).getint();
                    if (nKeysCount < 0 || nKeysCount > MAX_PUBKEYS_PER_MULTISIG)
                        return set_error(serror, SCRIPT_ERR_PUBKEY_COUNT);
                    nOpCount += nKeysCount;
                    if (!protocol_cleanup && (sigversion == SIGVERSION_BASE) && (nOpCount > MAX_OPS_PER_SCRIPT))
                        return set_error(serror, SCRIPT_ERR_OP_COUNT);
                    int ikey = ++i;
                    // ikey2 is the position of last non-signature item in the stack. Top stack item = 1.
                    // With SCRIPT_VERIFY_NULLFAIL, this is used for cleanup if operation fails.
                    int ikey2 = nKeysCount + 2;
                    i += nKeysCount;
                    if ((int)stack.size() < i)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    int nSigsCount = CScriptNum(stacktop(-i), fRequireMinimal).getint();
                    if (nSigsCount < 0 || nSigsCount > nKeysCount)
                        return set_error(serror, SCRIPT_ERR_SIG_COUNT);
                    int isig = ++i;
                    i += nSigsCount;
                    if ((int)stack.size() < i)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    // Subset of script starting at the most recent codeseparator
                    CScript scriptCode(pbegincodehash, pend);

                    // Drop the signature in pre-segwit scripts but not segwit scripts
                    // While we are iterating through the signatures, we record whether
                    // any of the signatures are empty, as this determines the allowed
                    // values for the MULTISIG_HINT field.
                    bool empty_sigs = false;
                    for (int k = 0; k < nSigsCount; k++)
                    {
                        valtype& vchSig = stacktop(-isig-k);
                        if (sigversion == SIGVERSION_BASE) {
                            scriptCode.FindAndDelete(CScript(vchSig));
                        }
                        empty_sigs = empty_sigs || vchSig.empty();
                    }

                    // A bug in the original CHECKMULTISIG implementation caused an
                    // extra item to be popped off the stack upon completion. When
                    // SCRIPT_VERIFY_MULTISIG_HINT is in effect, this otherwise unused
                    // parameter is a bitfield indicating which keys are NOT used. With
                    // this hint we can avoid expensive signature validation checks that
                    // might fail.
                    MultiSigHint hint(nKeysCount); // defaults to no-skipped-keys
                    if (enforce_multisig_hint) {
                        // There cannot be more than 20 keys, so our serialized
                        // hint cannot be more than 20 unsigned bits, which fits
                        // fine inside a 3-byte signed CScriptNum.
                        CScriptNum ser_hint(stacktop(-i), true, 3);
                        // Make sure that bits is within the numeric range of integers
                        // corresponding to our bitfield length, so that we don't risk
                        // any malleability or implementation / platform-defined
                        // behavior.
                        if ((ser_hint < 0) || (ser_hint >= (1 << nKeysCount))) {
                            return set_error(serror, SCRIPT_ERR_MULTISIG_HINT);
                        }
                        // Fill the skip-bitfield of our MultiSigHint object
                        hint << ser_hint;
                        // For a k-of-n multisig, there must be k signatures present and
                        // (n-k) keys marked unused. We require that the skip-bits for
                        // these keys be set in the hint's skipped_keys field. Note in
                        // particular that the corresponding bits must be set for keys
                        // in the final positions, if unused, even though the signature
                        // verification loop below terminates early in that situation.
                        if (hint.count_sigs() != (empty_sigs ? 0 : nSigsCount)) {
                            return set_error(serror, SCRIPT_ERR_MULTISIG_HINT);
                        }
                    }

                    bool fSuccess = true;
                    while (fSuccess && nSigsCount > 0)
                    {
                        valtype& vchSig    = stacktop(-isig);
                        valtype& vchPubKey = stacktop(-ikey);

                        // Note how this makes the exact order of pubkey/signature evaluation
                        // distinguishable by CHECKMULTISIG NOT if the STRICTENC flag is set.
                        // See the script_(in)valid tests for details.
                        if (!CheckSignatureEncoding(vchSig, flags, serror) || !CheckPubKeyEncoding(vchPubKey, flags, sigversion, serror)) {
                            // serror is set
                            return false;
                        }

                        // The first pubkey is at position (-ikey == -2),
                        // which is bit index 0 of hint's skipped_keys.
                        bool have_sig = hint.have_sig_for_key(ikey-2);

                        // Check signature
                        bool fOk = have_sig && checker.CheckSig(vchSig, vchPubKey, scriptCode, sigversion);

                        // Skipped keys MUST be reported in the hint if
                        // SCRIPT_VERIFY_MULTISIG_HINT is in effect.
                        if (!fOk && enforce_multisig_hint && have_sig) {
                            return set_error(serror, SCRIPT_ERR_FAILED_SIGNATURE_CHECK);
                        }

                        if (fOk) {
                            isig++;
                            nSigsCount--;
                        }
                        ikey++;
                        nKeysCount--;

                        // If there are more signatures left than keys left,
                        // then too many signatures have failed. Exit early,
                        // without checking any further signatures.
                        if (nSigsCount > nKeysCount)
                            fSuccess = false;
                    }

                    // Clean up stack of actual arguments
                    while (i-- > 1) {
                        // If the operation failed, we require that all signatures must be empty vector
                        if (!fSuccess && enforce_nullfail && !ikey2 && stacktop(-1).size())
                            return set_error(serror, SCRIPT_ERR_NULLFAIL);
                        if (ikey2 > 0)
                            ikey2--;
                        popstack(stack);
                    }

                    // A bug causes CHECKMULTISIG to consume one extra argument
                    // whose contents were not checked in any way.
                    //
                    // Unfortunately this is a potential source of mutability,
                    // so optionally verify it is exactly equal to zero prior
                    // to removing it from the stack.
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    popstack(stack);

                    stack.push_back(fSuccess ? vchTrue : vchFalse);

                    if (opcode == OP_CHECKMULTISIGVERIFY)
                    {
                        if (fSuccess)
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_CHECKMULTISIGVERIFY);
                    }
                }
                break;

                case OP_VERIF:
                case OP_VERNOTIF:
                {
                    if (protocol_cleanup || (sigversion != SIGVERSION_BASE)) {
                        // Because OP_VERIF and OP_VERNOTIF fall between OP_IF
                        // and OP_ENDIF, they are treated the same as the other
                        // conditionals: they are always evaluated, even within
                        // a non-executed IF/ELSE branch.

                        // So in the original script, decoding a OP_VERIF always
                        // resulted in SCRIPT_ERR_BAD_OPCODE, regardless of the
                        // value of fExec, much like the disabled opcodes.

                        // But post-cleanup or within a witness script, we want
                        // OP_VERIF and OP_VERNOTIF to be like any of the other
                        // as-yet undefined, "return true" opcodes, which DO NOT
                        // abort execution if decoded but not executed.
                        if (!fExec) {
                            break;
                        }
                    }
                }

                // Otherwise we fall through to the default case. (Either we're
                // in a live branch, or we're in a pre-segwit script; either
                // way, we handle these opcodes the same as any other undefined
                // opcode.)

                default:
                    if (!protocol_cleanup && (sigversion == SIGVERSION_BASE)) {
                        return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
                    }
                    if (discourage_upgradable_nops) {
                        return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                    }
                    altstack.clear();
                    stack.clear();
                    stack.push_back(vchTrue);
                    return set_success(serror);
            }

            // Size limits
            if (!protocol_cleanup && (sigversion == SIGVERSION_BASE) && (stack.size() + altstack.size() > MAX_STACK_SIZE))
                return set_error(serror, SCRIPT_ERR_STACK_SIZE);
            if ((stack.size() + altstack.size()) > MAX_WITNESS_STACK_SIZE) {
                return set_error(serror, SCRIPT_ERR_STACK_SIZE);
            }
        }
    }
    catch (...)
    {
        return set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    }

    if (!vfExec.empty())
        return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);

    return set_success(serror);
}

namespace {

/**
 * Wrapper that serializes like CTransaction, but with the modifications
 *  required for the signature hash done in-place
 */
class CTransactionSignatureSerializer {
private:
    const CTransaction& txTo;  //!< reference to the spending transaction (the one being serialized)
    const CScript& scriptCode; //!< output script being consumed
    const unsigned int nIn;    //!< input index of txTo being signed
    const bool fAnyoneCanPay;  //!< whether the hashtype has the SIGHASH_ANYONECANPAY flag set
    const bool fHashSingle;    //!< whether the hashtype is SIGHASH_SINGLE
    const bool fHashNone;      //!< whether the hashtype is SIGHASH_NONE
    const bool no_lock_height; //!< whether the hashtype has the SIGHASH_NO_LOCK_HEIGHT flag set

public:
    CTransactionSignatureSerializer(const CTransaction &txToIn, const CScript &scriptCodeIn, unsigned int nInIn, int nHashTypeIn) :
        txTo(txToIn), scriptCode(scriptCodeIn), nIn(nInIn),
        fAnyoneCanPay(!!(nHashTypeIn & SIGHASH_ANYONECANPAY)),
        fHashSingle((nHashTypeIn & 0x1f) == SIGHASH_SINGLE),
        fHashNone((nHashTypeIn & 0x1f) == SIGHASH_NONE),
        no_lock_height(!!(nHashTypeIn & SIGHASH_NO_LOCK_HEIGHT)) {}

    /** Serialize the passed scriptCode, skipping OP_CODESEPARATORs */
    template<typename S>
    void SerializeScriptCode(S &s) const {
        CScript::const_iterator it = scriptCode.begin();
        CScript::const_iterator itBegin = it;
        opcodetype opcode;
        unsigned int nCodeSeparators = 0;
        while (scriptCode.GetOp(it, opcode)) {
            if (opcode == OP_CODESEPARATOR)
                nCodeSeparators++;
        }
        ::WriteCompactSize(s, scriptCode.size() - nCodeSeparators);
        it = itBegin;
        while (scriptCode.GetOp(it, opcode)) {
            if (opcode == OP_CODESEPARATOR) {
                s.write((char*)&itBegin[0], it-itBegin-1);
                itBegin = it;
            }
        }
        if (itBegin != scriptCode.end())
            s.write((char*)&itBegin[0], it-itBegin);
    }

    /** Serialize an input of txTo */
    template<typename S>
    void SerializeInput(S &s, unsigned int nInput) const {
        // In case of SIGHASH_ANYONECANPAY, only the input being signed is serialized
        if (fAnyoneCanPay)
            nInput = nIn;
        // Serialize the prevout
        ::Serialize(s, txTo.vin[nInput].prevout);
        // Serialize the script
        if (nInput != nIn)
            // Blank out other inputs' signatures
            ::Serialize(s, CScript());
        else
            SerializeScriptCode(s);
        // Serialize the nSequence
        if (nInput != nIn && (fHashSingle || fHashNone))
            // let the others update at will
            ::Serialize(s, (int)0);
        else
            ::Serialize(s, txTo.vin[nInput].nSequence);
    }

    /** Serialize an output of txTo */
    template<typename S>
    void SerializeOutput(S &s, unsigned int nOutput) const {
        if (fHashSingle && nOutput != nIn)
            // Do not lock-in the txout payee at other indices as txin
            ::Serialize(s, CTxOut());
        else
            ::Serialize(s, txTo.vout[nOutput]);
    }

    /** Serialize txTo */
    template<typename S>
    void Serialize(S &s) const {
        // Serialize nVersion
        ::Serialize(s, txTo.nVersion);
        // Serialize vin
        unsigned int nInputs = fAnyoneCanPay ? 1 : txTo.vin.size();
        ::WriteCompactSize(s, nInputs);
        for (unsigned int nInput = 0; nInput < nInputs; nInput++)
             SerializeInput(s, nInput);
        // Serialize vout
        unsigned int nOutputs = fHashNone ? 0 : (fHashSingle ? nIn+1 : txTo.vout.size());
        ::WriteCompactSize(s, nOutputs);
        for (unsigned int nOutput = 0; nOutput < nOutputs; nOutput++)
             SerializeOutput(s, nOutput);
        // Serialize nLockTime
        ::Serialize(s, txTo.nLockTime);
        // Serialize lock_height
        if (!no_lock_height && (txTo.nVersion != 1 || txTo.vin.size() != 1 || !txTo.vin[0].prevout.IsNull())) {
            ::Serialize(s, txTo.lock_height);
        }
    }
};

uint256 GetPrevoutHash(const CTransaction& txTo) {
    CHashWriter ss(SER_GETHASH, 0);
    for (const auto& txin : txTo.vin) {
        ss << txin.prevout;
    }
    return ss.GetHash();
}

uint256 GetSequenceHash(const CTransaction& txTo) {
    CHashWriter ss(SER_GETHASH, 0);
    for (const auto& txin : txTo.vin) {
        ss << txin.nSequence;
    }
    return ss.GetHash();
}

uint256 GetOutputsHash(const CTransaction& txTo) {
    CHashWriter ss(SER_GETHASH, 0);
    for (const auto& txout : txTo.vout) {
        ss << txout;
    }
    return ss.GetHash();
}

} // namespace

PrecomputedTransactionData::PrecomputedTransactionData(const CTransaction& txTo)
{
    hashPrevouts = GetPrevoutHash(txTo);
    hashSequence = GetSequenceHash(txTo);
    hashOutputs = GetOutputsHash(txTo);
}

uint256 SignatureHash(const CScript& scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType, const CAmount& amount, int64_t refheight, SigVersion sigversion, const PrecomputedTransactionData* cache)
{
    if (sigversion == SIGVERSION_WITNESS_V0) {
        uint256 hashPrevouts;
        uint256 hashSequence;
        uint256 hashOutputs;

        if (!(nHashType & SIGHASH_ANYONECANPAY)) {
            hashPrevouts = cache ? cache->hashPrevouts : GetPrevoutHash(txTo);
        }

        if (!(nHashType & SIGHASH_ANYONECANPAY) && (nHashType & 0x1f) != SIGHASH_SINGLE && (nHashType & 0x1f) != SIGHASH_NONE) {
            hashSequence = cache ? cache->hashSequence : GetSequenceHash(txTo);
        }


        if ((nHashType & 0x1f) != SIGHASH_SINGLE && (nHashType & 0x1f) != SIGHASH_NONE) {
            hashOutputs = cache ? cache->hashOutputs : GetOutputsHash(txTo);
        } else if ((nHashType & 0x1f) == SIGHASH_SINGLE && nIn < txTo.vout.size()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << txTo.vout[nIn];
            hashOutputs = ss.GetHash();
        }

        CHashWriter ss(SER_GETHASH, 0);
        // Version
        ss << txTo.nVersion;
        // Input prevouts/nSequence (none/all, depending on flags)
        ss << hashPrevouts;
        ss << hashSequence;
        // The input being signed (replacing the scriptSig with scriptCode + amount)
        // The prevout may already be contained in hashPrevout, and the nSequence
        // may already be contain in hashSequence.
        ss << txTo.vin[nIn].prevout;
        ss << scriptCode;
        ss << amount;
        if (!(nHashType & SIGHASH_NO_LOCK_HEIGHT)) {
            ss << refheight;
        }
        ss << txTo.vin[nIn].nSequence;
        // Outputs (none/one/all, depending on flags)
        ss << hashOutputs;
        // Locktime
        ss << txTo.nLockTime;
        // Lockheight
        if (!(nHashType & SIGHASH_NO_LOCK_HEIGHT)) {
            ss << txTo.lock_height;
        }
        // Sighash type
        ss << (nHashType & ~SIGHASH_NO_LOCK_HEIGHT);

        return ss.GetHash();
    }

    static const uint256 one(uint256S("0000000000000000000000000000000000000000000000000000000000000001"));
    if (nIn >= txTo.vin.size()) {
        //  nIn out of range
        return one;
    }

    // Check for invalid use of SIGHASH_SINGLE
    if ((nHashType & 0x1f) == SIGHASH_SINGLE) {
        if (nIn >= txTo.vout.size()) {
            //  nOut out of range
            return one;
        }
    }

    // Wrapper to serialize only the necessary parts of the transaction being signed
    CTransactionSignatureSerializer txTmp(txTo, scriptCode, nIn, nHashType);

    // Serialize and hash
    CHashWriter ss(SER_GETHASH, 0);
    ss << txTmp << (nHashType & ~SIGHASH_NO_LOCK_HEIGHT);
    return ss.GetHash();
}

bool TransactionSignatureChecker::VerifySignature(const std::vector<unsigned char>& vchSig, const CPubKey& pubkey, const uint256& sighash) const
{
    return pubkey.Verify(sighash, vchSig);
}

bool TransactionSignatureChecker::CheckSig(const std::vector<unsigned char>& vchSigIn, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
{
    CPubKey pubkey(vchPubKey);
    if (!pubkey.IsValid())
        return false;

    // Hash type is one byte tacked on to the end of the signature
    std::vector<unsigned char> vchSig(vchSigIn);
    if (vchSig.empty())
        return false;
    int nHashType = vchSig.back();
    vchSig.pop_back();

    // If we are in bitcoin compatibility mode, then we must pass on a
    // flag to indicate that the lock_height field of the transaction
    // must not be serialized during the signature check. This feature
    // is present for the sole purpose of supporting validation of
    // signatures encoded within unit tests carried over from the
    // bitcoin code base.
    if (no_lock_height)
        nHashType |= SIGHASH_NO_LOCK_HEIGHT;

    uint256 sighash = SignatureHash(scriptCode, *txTo, nIn, nHashType, amount, refheight, sigversion, this->txdata);

    if (!VerifySignature(vchSig, pubkey, sighash))
        return false;

    return true;
}

bool TransactionSignatureChecker::CheckLockTime(const CScriptNum& nLockTime) const
{
    // There are two kinds of nLockTime: lock-by-blockheight
    // and lock-by-blocktime, distinguished by whether
    // nLockTime < LOCKTIME_THRESHOLD.
    //
    // We want to compare apples to apples, so fail the script
    // unless the type of nLockTime being tested is the same as
    // the nLockTime in the transaction.
    if (!(
        (txTo->nLockTime <  LOCKTIME_THRESHOLD && nLockTime <  LOCKTIME_THRESHOLD) ||
        (txTo->nLockTime >= LOCKTIME_THRESHOLD && nLockTime >= LOCKTIME_THRESHOLD)
    ))
        return false;

    // Now that we know we're comparing apples-to-apples, the
    // comparison is a simple numeric one.
    if (nLockTime > (int64_t)txTo->nLockTime)
        return false;

    // Finally the nLockTime feature can be disabled and thus
    // CHECKLOCKTIMEVERIFY bypassed if every txin has been
    // finalized by setting nSequence to maxint. The
    // transaction would be allowed into the blockchain, making
    // the opcode ineffective.
    //
    // Testing if this vin is not final is sufficient to
    // prevent this condition. Alternatively we could test all
    // inputs, but testing just this input minimizes the data
    // required to prove correct CHECKLOCKTIMEVERIFY execution.
    if (CTxIn::SEQUENCE_FINAL == txTo->vin[nIn].nSequence)
        return false;

    return true;
}

bool TransactionSignatureChecker::CheckSequence(const CScriptNum& nSequence) const
{
    // Relative lock times are supported by comparing the passed
    // in operand to the sequence number of the input.
    const int64_t txToSequence = (int64_t)txTo->vin[nIn].nSequence;

    // Sequence numbers with their most significant bit set are not
    // consensus constrained. Testing that the transaction's sequence
    // number do not have this bit set prevents using this property
    // to get around a CHECKSEQUENCEVERIFY check.
    if (txToSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG)
        return false;

    // Mask off any bits that do not have consensus-enforced meaning
    // before doing the integer comparisons
    const uint32_t nLockTimeMask = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | CTxIn::SEQUENCE_LOCKTIME_MASK;
    const int64_t txToSequenceMasked = txToSequence & nLockTimeMask;
    const CScriptNum nSequenceMasked = nSequence & nLockTimeMask;

    // There are two kinds of nSequence: lock-by-blockheight
    // and lock-by-blocktime, distinguished by whether
    // nSequenceMasked < CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG.
    //
    // We want to compare apples to apples, so fail the script
    // unless the type of nSequenceMasked being tested is the same as
    // the nSequenceMasked in the transaction.
    if (!(
        (txToSequenceMasked <  CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG && nSequenceMasked <  CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) ||
        (txToSequenceMasked >= CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG && nSequenceMasked >= CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG)
    )) {
        return false;
    }

    // Now that we know we're comparing apples-to-apples, the
    // comparison is a simple numeric one.
    if (nSequenceMasked > txToSequenceMasked)
        return false;

    return true;
}

static bool VerifyWitnessProgram(const CScriptWitness& witness, int witversion, const std::vector<unsigned char>& program, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* serror)
{
    std::vector<std::vector<unsigned char> > stack;
    CScript scriptPubKey;

    if (witversion == 0) {
        if ((program.size() == 20) || (program.size() == 32)) {
            // Version 0 segregated witness program: Merkle root inside the
            // program, Merkle proof + CScript + inputs in witness
            if (witness.stack.size() <= 1) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY);
            }
            // The Merkle proof is a minimally-serialized Merkle branch
            // consisting of a bitfield N bits long (the path) and 32*N hashes.
            // The maximum supported depth of the tree is 33 layers, including
            // the root.
            const std::vector<unsigned char>& proof_field = witness.stack.back();
            if (proof_field.size() > 1028) { // 1028 = 32*32 + (32/8)
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_INVALID_PROOF);
            }
            const int bytes_in_path = proof_field.size() % 32;
            const int max_bytes_in_path = ((proof_field.size() / 32) + 7) / 8;
            if (bytes_in_path > max_bytes_in_path) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_INVALID_PROOF);
            }
            if (bytes_in_path && (proof_field[bytes_in_path-1] == 0)) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_INVALID_PROOF);
            }
            uint32_t path = 0;
            switch (bytes_in_path) {
                case 4: path |= (static_cast<uint32_t>(proof_field[3]) << 24);
                case 3: path |= (static_cast<uint32_t>(proof_field[2]) << 16);
                case 2: path |= (static_cast<uint32_t>(proof_field[1]) <<  8);
                case 1: path |=  static_cast<uint32_t>(proof_field[0]);
                case 0: break;
                default:
                    return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_INVALID_PROOF);
            }
            std::vector<uint256> branch;
            branch.reserve(proof_field.size() / 32);
            for (auto ptr = proof_field.begin() + bytes_in_path; ptr != proof_field.end(); ptr += 32) {
                branch.emplace_back(std::vector<unsigned char>(ptr, ptr + 32));
            }
            const std::vector<unsigned char>& script_field = *(witness.stack.end() - 2);
            uint256 hashScriptPubKey;
            CHash256().Write(&script_field[0], script_field.size()).Finalize(hashScriptPubKey.begin());
            bool invalid = false;
            hashScriptPubKey = ComputeFastMerkleRootFromBranch(hashScriptPubKey, branch, path, &invalid);
            if (invalid) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_INVALID_PROOF);
            }
            if (program.size() == 20) {
                CRIPEMD160().Write(hashScriptPubKey.begin(), 32).Finalize(hashScriptPubKey.begin());
            }
            if (memcmp(hashScriptPubKey.begin(), &program[0], program.size())) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
            }
            if (!script_field.empty() && script_field[0] == 0x00) {
                scriptPubKey = CScript(script_field.begin()+1, script_field.end());
                stack = std::vector<std::vector<unsigned char> >(witness.stack.begin(), witness.stack.end() - 2);
            } else if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM) {
                return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM);
            } else {
                // Higher inner-version witness scripts return true for future soft-fork compatibility.
                return set_success(serror);
            }
        } else if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM) {
            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM);
        } else {
            // Unrecognized payload lengths return true for future softfork compatibility
            return set_success(serror);
        }
    } else if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM) {
        return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM);
    } else {
        // Higher outer-version witness scripts return true for future soft-fork compatibility
        return set_success(serror);
    }

    // Disallow more then MAX_WITNESS_STACK_SIZE elements in witness stack
    if (stack.size() > MAX_WITNESS_STACK_SIZE) {
        return set_error(serror, SCRIPT_ERR_STACK_SIZE);
    }

    if (!EvalScript(stack, scriptPubKey, flags, checker, SIGVERSION_WITNESS_V0, serror)) {
        return false;
    }

    if (stack.empty())
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
    if (!CastToBool(stack.back()))
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
    return true;
}

bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* serror)
{
    static const CScriptWitness emptyWitness;
    if (witness == nullptr) {
        witness = &emptyWitness;
    }
    bool hadWitness = false;

    set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);

    if ((flags & SCRIPT_VERIFY_SIGPUSHONLY) != 0 && !scriptSig.IsPushOnly()) {
        return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);
    }

    std::vector<std::vector<unsigned char> > stack, stackCopy;
    if (!EvalScript(stack, scriptSig, flags, checker, SIGVERSION_BASE, serror))
        // serror is set
        return false;
    if (flags & SCRIPT_VERIFY_P2SH)
        stackCopy = stack;
    if (!EvalScript(stack, scriptPubKey, flags, checker, SIGVERSION_BASE, serror))
        // serror is set
        return false;
    if (stack.empty())
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
    if (CastToBool(stack.back()) == false)
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);

    // Bare witness programs
    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (flags & SCRIPT_VERIFY_WITNESS) {
        if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            hadWitness = true;
            if (scriptSig.size() != 0) {
                // The scriptSig must be _exactly_ CScript(), otherwise we reintroduce malleability.
                return set_error(serror, SCRIPT_ERR_WITNESS_MALLEATED);
            }
            if (!VerifyWitnessProgram(*witness, witnessversion, witnessprogram, flags, checker, serror)) {
                return false;
            }
            // Bypass the cleanstack check at the end. The actual stack is obviously not clean
            // for witness programs.
            stack.resize(1);
        }
    }

    // Additional validation for spend-to-script-hash transactions:
    if ((flags & SCRIPT_VERIFY_P2SH) && scriptPubKey.IsPayToScriptHash())
    {
        // scriptSig must be literals-only or validation fails
        if (!scriptSig.IsPushOnly())
            return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);

        // Restore stack.
        swap(stack, stackCopy);

        // stack cannot be empty here, because if it was the
        // P2SH  HASH <> EQUAL  scriptPubKey would be evaluated with
        // an empty stack and the EvalScript above would return false.
        assert(!stack.empty());

        const valtype& pubKeySerialized = stack.back();
        CScript pubKey2(pubKeySerialized.begin(), pubKeySerialized.end());
        popstack(stack);

        if (!EvalScript(stack, pubKey2, flags, checker, SIGVERSION_BASE, serror))
            // serror is set
            return false;
        if (stack.empty())
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
        if (!CastToBool(stack.back()))
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
    }

    if (flags & SCRIPT_VERIFY_WITNESS) {
        // We can't check for correct unexpected witness data if P2SH was off, so require
        // that WITNESS implies P2SH. Otherwise, going from WITNESS->P2SH+WITNESS would be
        // possible, which is not a softfork.
        assert((flags & SCRIPT_VERIFY_P2SH) != 0);
        if (!hadWitness && !witness->IsNull()) {
            return set_error(serror, SCRIPT_ERR_WITNESS_UNEXPECTED);
        }
    }

    // The CLEANSTACK check is only performed after potential P2SH evaluation,
    // as the non-P2SH evaluation of a P2SH script will obviously not result in
    // a clean stack (the P2SH inputs remain). The same holds for witness evaluation.
    if ((flags & SCRIPT_VERIFY_CLEANSTACK) != 0) {
        // Disallow CLEANSTACK without P2SH, as otherwise a switch CLEANSTACK->P2SH+CLEANSTACK
        // would be possible, which is not a softfork (and P2SH should be one).
        assert((flags & SCRIPT_VERIFY_P2SH) != 0);
        assert((flags & SCRIPT_VERIFY_WITNESS) != 0);
        if (stack.size() != 1) {
            return set_error(serror, SCRIPT_ERR_CLEANSTACK);
        }
    }

    return set_success(serror);
}
