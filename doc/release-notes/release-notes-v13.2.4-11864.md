Freicoin version v13.2.4-11864 is now available from:

  * [Linux i686 (Intel 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-i686-pc-linux-gnu.tar.gz)
  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-aarch64-linux-gnu.tar.gz)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4-11864-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v13.2.4-11864.zip)

This is a new patch release, containing the auxiliary proof-of-work (AKA "merge
mining") soft-fork consensus code and activation parameters, as well as various
bug fixes to the internal stratum server implementation.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tradecraftio/tradecraft/issues>

To receive security and update notifications, please subscribe to:

  <https://tradecraft.groups.io/g/announce/>

Notable changes
===============

The recently released v13.2-11780 features activation parameters for the
segregated witness (segwit) soft-fork, and dropped support for Microsoft
Windows XP and macOS 10.7.  Please see the comprehensive notes accompanying
v13.2-11780 for more information.

Auxiliary proof-of-work soft-fork
---------------------------------

Release v13.2.4-11864 adds support for so-called auxiliary proof-of-work (AKA
"merge-mining") using bitcoin-compatible SHA256d, as a soft-fork.  Once
activated, it allows miners to simultaneously work on both Bitcoin and
Freicoin/Tradecraft blocks at the same time, finding proof-of-work solutions
that satisfy one or the other, or both blocks.  It also adds poolside protection
against block withholding attacks, allowing miners who opt into these new
features to prevent their workers from withholding winning shares.

The only previously described soft-fork proof-of-work change is the Forward
Blocks proposal.  This is a simplification of the ideas presented therein, which
achieves the soft-fork proof-of-work change in a way that can be deployed more
quickly, but is still compatible with future deployment of Forward Blocks.

The basic idea is that we require each new block to satisfy two proof of work
challenges in series: first the auxiliary proof-of-work (merge-mining)
challenge, which can be shared across multiple chains including bitcoin, then
the existing (native) header challenge which specific to our chain.  Thus the
order of work is that a miner first generates a Freicoin/Tradecraft block
template, then commits to this block template in a special location within a
bitcoin block.  The miner then sets about solving that block.  Each returned
share is checked to see if it satisfies Tradecraft's auxiliary proof-of-work
requirement.  When a winning share is found, the block template is modified to
commit to the auxiliary proof-of-work, and it is returned to miners to have its
native proof-of-work solved.

By having the auxiliary proof-of-work difficulty adjustment algorithm target a
longer inter-block period, there will be a transition period during which the
difficulty of the native header adjusts down and the auxiliary difficulty
adjusts up.  During this period the overall proof-of-work function will not be
progress-free, but the transition period will be short (a few months) and the
benefits that come with merged mining well worth it.  At the end of the
transition period, the native/compatibility headers will be at or near
difficulty 1, which is effectively progress-free, or practically no limit at all
once the protocol-cleanup fork activates.

The parent chain contains a commitment to the Tradecraft block at a fixed
location.  The native chain likewise commits to the auxiliary proof-of-work in a
fixed location as well.  The block header structure is modified to contain the
information needed to validate these commitments.

The merge mining commitment in the parent chain is placed in the last 36 bytes
of the last output of the last transaction.  This helps to minimize the size of
the auxiliary header's Merkle proof.  The commitment format is a 32-byte
commitment hash, a 4-byte commitment identifier, and the transaction's
`nLockTime` field.  Note that no `lock_height` field is present, so Bitcoin can
act as the parent chain, but the commitment identifier is chosen so that
Tradecraft cannot.  This allows merge mining with Bitcoin and prevents
Tradecraft blocks from containing such a commitment.

Note: since Bitcoin does not have consensus-enforced block-final transactions,
and since every Bitcoin transaction requires an input, to merge-mine Tradecraft
blocks a miner must have at least one input they control.

The commitment structure includes a hash of a pool-selected secret value, which
is used for block-withholding protection.  The pool can allow auxiliary
difficulty to be split across two challenges, one which is calculated in the
usual way that is compatible with bitcoin, and the other using the
block-withholding secret.  This serves two purposes: (1) a miner which submits a
share cannot predict whether it will be a valid block or not, and therefore
cannot grief a pool through block withholding, and (2) it decouples the finding
of Tradecraft blocks from bitcoin blocks, since a share that meets the necessary
work threshold for both chains isn't necessarily accepted by both.

Once merged mining is activated, every block has two proof-of-work targets: the
native difficulty and the auxiliary (merge mined) difficulty.  And unlike
previous merge-mining schemes like namecoin's, every block MUST have an
auxiliary header: merge mining is not optional on a block-by-block basis.

Both the native and auxiliary difficulty adjustment algorithms separately track
observed hash rate according to the block timestamps and their expected block
arrival predictors, except that the auxiliary difficulty adjustment algorithm
targets a longer inter-block interval: 15 minutes / 900 seconds, which results
in 4 blocks per hour, or 96 blocks per day.

Once the auxiliary proof is satisfied, a proof then needs to be found for the
native header.  The miner who finds this proof is allowed to change the block
template coinbase's scriptSig or nSequence field, and/or the header's nNonce and
nVersion fields.  Note that the native header's nTime field MUST be the same as
the auxiliary header's, so nTime-rolling isn't permitted when solving the native
header.

These changes are supported by the built-in stratum mining server, which will
serve non-merge-mined work to connected miners in the default configuration.
That is to say, it constructs a fake "bitcoin" block for the auxiliary
proof-of-work.  Work is ongoing to modify Bitcoin Core so as to support merge
mining of Tradecraft blocks (no consensus changes are needed for bitcoin).

New difficulty adjustment algorithm
-----------------------------------

As part of the auxiliary proof-of-work soft-fork, a new algorithm is implemented
for adjusting the target / difficulty of the auxiliary proof-of-work.  The new
algorithm adjusts difficulty every block, using a 2nd order bessel filter to
estimate the current network hashrate.  This new adjustment algorithm is able to
much more quickly and accurately track the actual network hash rate, and should
reduce the incidence of cyclical swings in difficulty.

Split scheduled hard-forks into "protocol cleanup" and "size expansion"
-----------------------------------------------------------------------

There was a previously scheduled "protocol cleanup" hard-fork which would have
provided a number of navel-gazing fixes to the consensus rules when activated,
in 2022 at the earliest, as well as relaxing the aggregate block and transaction
limits.  The switch to merge mining, however, presents a unique opportunity to
have an accelerated hard-fork schedule for those rule relaxations which are
totally uncontroversial.  This PR therefore splits the previous hard-fork into
two separate flag-day events, the non-controversial protocol-cleanup fixes
scheduled for some months after the activation of merged mining, and the block
size expansion which remains scheduled for 2+ years in the future.

Protocol-cleanup hard-fork
--------------------------

Merge mining is implemented as a soft-fork change to the consensus rules to
achieve a safer and less-disruptive deployment than the hard-fork that was used
to deploy merge mining to other chains in the past: non-upgraded clients will
continue to receive blocks at the point of activation with SPV security.
However the security of successive blocks will diminish as the difficulty
transitions from native to auxiliary proof-of-work.  When the native difficulty
reaches minimal values, there will no longer be any effective SPV protections
for old nodes, and this represents a unique opportunity to deploy other
non-controversial hard-fork changes to the consensus rules.

For this reason, a protocol-cleanup hard-fork is scheduled to take place after
the activation of merge mining and the transition of difficulty.  As part of
this cleanup, the following consensus rule changes will take effect:

1. Remove the native proof-of-work requirement entirely.  For reasons of
   infrastructure compatibility the block hash will still be the hash of the
   native header, but the hash of the header will no longer have to meet any
   threshold target.  This makes every winning auxiliary share a valid block
   without any extra work.

2. Remove the `MAX_BLOCK_SIGOPS_COST` limit.  Switching to libsecp256k1 for
   validation and better signature / script and transaction validation caching
   has made this limit nearly redundant.

3. Allow transactions without transaction outputs.  A transaction must have
   input(s) to have a unique transaction ID, but it need not have any outputs.
   There are obscure cases when this makes sense to do (and thus forward the
   funds entirely as "fee" to the miner, or to process in the block-final
   transaction and/or coinbase in some way).

4. Do not restrict the contents of the "coinbase string" in any way, beyond the
   required auxiliary proof-of-work commitment.  It is currently required to be
   between 2 and 100 bytes in size, and must begin with the serialized block
   height.  The length restriction is unnecessary as miners have other means of
   padding transactions if they need to, and are generally incentivized not to
   because of miner fees.  The serialized height requirement is redundant as
   lock_height is also required to be set to the current block height.

5. Do not require the coinbase transaction to be final, freeing up nSequence to
   be used as the miner's extranonce field.  A previous soft-fork which required
   the coinbase's nLockTime field to be set to the medium-time-past value had
   the unfortunate side effect of requiring nSequence to be set to 0xffffffff
   since even the coinbase is checked for transaction finality.  The concept of
   finality makes no sense for the coinbase and this requirement is dropped
   after activation of the new rules, making the 4-byte nSequence field have no
   consensus-defined meaning, allowing it to be used as an extranonce field.

6. Do not require zero-valued outputs to be spent by transactions with
   lock_height >= the coin's refheight.  This restriction was to ensure that
   refheights are always increasing so that demurrage is collected, not
   reversed.  However this argument doesn't really make sense for zero-valued
   outputs.  At the same time, "zero-valued" outputs are increasingly likely to
   be used for confidential transactions or non-freicoin issued assets using
   extension outputs, for which the monotonic lock_height requirement is just an
   annoying protocol complication.

7. Do not reject "old" blocks after activation of the nVersion=2 and nVersion=3
   soft-forks.  With the switch to version bits for soft-fork activation, this
   archaic check is shown to be rather pointless.  Rules are enforced in a block
   if it is downstream of the point of activation, not based on the nVersion
   value.  Implicitly this also restores validity of "negative" block.nVersion
   values.

8. Lift restrictions inside the script interpreter on maximum script size,
   maximum data push, maximum number of elements on the stack, and maximum
   number of executed opcodes.

9. Remove checks on disabled opcodes, and cause unrecognized opcodes to "return
   true" instead of raising an error.

10. Re-enable (and implement) certain disabled opcodes, and conspicuously
    missing opcodes which were never there in the first place.

Activation of the protocol-cleanup fork depends on the status of the auxpow
soft-fork, and the median-time-past of the tip relative to a consensus
parameter.  For main net, the auxpow soft-fork must be active, and the
median-time-past of the prior block be after midnight UTC on 16 Jan 2021.

Size-expansion hard-fork
------------------------

To achieve desired scaling limits, the forward blocks architecture will
eventually trigger a hard-fork modification of the consensus rules, for the
primary purpose of dropping enforcement of many aggregate block limits so as to
allow larger blocks on the compatibility chain.

This hard-fork will not activate until it is absolutely necessary for it to do
so, at the point when real demand for additional shard space in aggregate across
all forward block shard-chains exceeds the available space in the compatibility
chain.  It is anticipated that this will not occur until many, many years into
the future, when Freicoin/Tradecraft's usage exceeds even the levels of bitcoin
usage ca. 2018.  However when it does eventually trigger, any node enforcing the
old rules will be left behind.

Since the rule changes for forward blocks have not been written yet and because
this flag-day code doesn't know how to detect actual activation, we cannot have
older clients enforce the new rules.  What is done instead is that any rule
which we anticipate changing becomes simply unenforced after this activation
time, and aggregate limits are set to the maximum values the software is able to
support.  After the flag-day, older clients of at least version 13.2.4 will
continue to receive blocks, but with only SPV security ("trust the most work")
for the new protocol rules.  So starting with the release of v13.2.4-11864,
activation of forward blocks' new scaling limits becomes a soft-fork, with the
only concern being the forking off of older nodes upon activation.

The primary rules which must be altered for forward blocks scaling are:

1. Significant relaxation of the rules regarding per-block auxiliary difficulty
   adjustment, to allow adjustments of +/- 2x within eleven blocks, without
   regard of a target interval.  Forward blocks may have a new difficulty
   adjustment algorithm that has yet to be determined, and might include
   targeting a variable inter-block time to achieve compatibility chain
   scalability.

2. Increase of the maximum block size.  Uncapping the block size is not possible
   because even if the explicit limit is removed there are still implicit
   network and disk protocol limits that would prevent a client from syncing a
   chain with larger blocks.  But these network and disk limits could be set
   much higher than the current limits based on a
   1 megabyte MAX_BASE_BLOCK_SIZE / 4 megaweight MAX_BLOCK_WEIGHT.

3. Allow larger transactions, up to the new, larger maximum block size limit in
   size.  This is less safe than increasing the block size since most of the
   nonlinear validation costs are quadratic in transaction size.  But there is
   research to be done in choosing what new limits should be used, and in the
   mean time keeping transactions only limited by the (new) block size permits
   flexibility in that future choice.

4. Reduce coinbase maturity to 1 block.  Once forward blocks has activated,
   coinbase maturity is an unnecessary delay to processing the coinbase payout
   queue.  It must be at least 1 to prevent miners from issuing themselves
   excess funds for the duration of 1 block.

Since we don't know when forward blocks will be deployed and activated, we
schedule these rule changes to occur at the end of the support window for each
client release, which is typically 2 years.  Each new release pushes back this
activation date, and since the new rules are a relaxation of the old rules older
clients will remain compatible so long as a majority of miners have upgrade and
thereby pushed back their activation dates.  When forward blocks is finally
deployed and activated, it will schedule its own modified rule relaxation to
occur after the most distant flag day.  For main net, this date will be no
earlier than midnight UTC on the 2nd of September, 2022.

Testnet
-------

Some of the core consensus rules of testnet needed to be modified in order to
have a timely activation of auxiliary proof-of-work rules for testing purpoes,
and so the network was restarted with new rules and a new genesis block.  Among
the changes made in the network reset is the removal of the special testnet rule
for low-diff mining, since this is largely obsoleted by merge mining, and the
versionbits interval is lowered from 2016 blocks to 504 for faster activation.

Stratum server
--------------

The stratum server no longer serves work to miners below the requested
difficulty.  Some work proxies (e.g. NiceHash) and some mining hardware will
refuse to do work at less than a minimum threshold.  Such clients specify their
minimum difficulty thresholds with the 'addr+diff' syntax in their username.
Previously the stratum server only respected this request if the asked-for
difficulty was less than the block's difficulty.  However with this new release
the miner will be given work at the requested share difficulty regardless of the
underlying block's target.  The reasoning is that it is better that the miner be
doing work, even inefficiently, than the miner go idle because of low difficulty
work requests.  This is especially true once the auxpow soft-fork activates and
difficulty transitions away from the native proof-of-work.

Notes for miners
================

The adoption of an auxiliary proof-of-work structure is a big change that
impacts all nearly levels of mining infrastructure, except for the mining
hardware itself.  Mining pool servers, proxies, and other infrastructure will
have to be adapted to work with auxiliary proof-of-work structures.

If you use the built-in stratum server, you need not do anything different to
continue mining Freicoin/Tradecraft.  You will *NOT* be merge-mining with
bitcoin, however!  To merge-mine with bitcoin you need to use a mining proxy
that is able to incorporate the Freicoin/Tradecraft merge-mining commitment into
the bitcoin block and check for winning auxiliary blocks.  THIS RELEASE DOES NOT
INCLUDE SUCH SOFTWARE.  It is anticipated that future releases will include, or
at the very least reference bitcoin mining proxy code that supports merge mining
of Freicoin/Tradecraft.  But such software does not yet exist, as it is being
written in parallel with deployment of the consensus layer changes.

Please consider signing up for the Tradecraft announcement list to receive
information about such software as soon as it is made available:

    <https://tradecraft.groups.io/g/announce/>

v13.2.4-11864 Change log
========================

The following pull requests were merged into the Tradecraft code repository
for this release.

- #84 Merge Mining in Tradecraft without Forward Blocks

  Adds merged mining with Bitcoin via the "auxiliary proof-of-work" soft-fork.

- #85 [Hard-Fork] Split scheduled hard-forks into "protocol cleanup" and "size
                  expansion"

  The protocol-cleanup and size-expansion hard-forks were previously just lumped
  together into one protocol-cleanup flag-day.  This PR splits them into two
  separate flag-day forks, and makes a few adjustments to reflect the new needs
  post-auxpow.

- #86 [MergeMining] Set auxpow difficulty to half native difficulty on
                    activation.

  In #84 the auxiliary proof-of-work difficulty starts out at a minimal value
  and then adjusts upward to fill a 15 minute expected block-time.  This would
  have caused a month-long delay for no reason, as it is trivial enough to set
  the auxiliary difficulty to the necessary value at the point of activation,
  which is what this PR does.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Kalle Alm
- Reza Erfani
- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
