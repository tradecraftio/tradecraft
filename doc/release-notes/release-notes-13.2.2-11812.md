Freicoin version v13.2.2-11812 is now available from:

  * [Linux i686 (Intel 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-i686-pc-linux-gnu.tar.gz)
  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-aarch64-linux-gnu.tar.gz)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.2-11812-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v13.2.2-11812.zip)

This is a new patch release, featuring various improvements to the stratum
server interface to fix bugs, better compatibility, and improved performance.

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

Stratum server
--------------

The initial response of the server to a `mining.subscribe` message is tweaked
to comply with the stratum mining protocol (for details, see the change log
below).  As this new behavior conforms to the expectations of existing mining
software, it is very unlikely that existing software relied on the prior
behavior.

Several performance improvements are included to make the stratum server
return work faster when block templates are large (e.g. full blocks).

A few small features were added to the stratum mining protocol to solve
compatibility issues with a number of mining clients.

v13.2.2-11812 Change log
========================

The following pull requests were merged into the Tradecraft code repository
for this release.

- #76 [Stratum] Prevent sending double stratum work notifications.

  When responding to a new block notification and sending updated work to the
  connected miners, ignore clients that are already working on the new block.
  Typically this is just the miner that found the block, who was immediately
  sent a work update at the time they submitted their winning share.

  In this release the stratum server is updated to avoid sending work to that
  miner again when the new-block notification is handled, moments later.  Due
  to race conditions there could be more than one miner that have already
  received an update, and if so those miners won't receive a second
  notification either.

  This change in behavior prevents unncessary work restarts by the miner.

- #78 [Merkle] Add stable Merkle branch proof APIs.

  The Satoshi Merkle tree design will duplicate hash values along the
  right-most branch of the tree if it is not a power of 2 in size.  In the
  regular Merkle branch APIs, these hashes are included in the branch proof.
  This not only wastes space, but is problematic because these duplicated
  hashes are dependent on the leaf value being proven, so the proof can't be
  used to recalculate a new root if the leaf value changes.

  This release adds a new internal API for generating Merkle tree branch
  proofs does not include duplicated hashes, so the result is both shorter (if
  it is along the right-hand side of an unbalanced tree) and can be safely
  used to recalculate root hash values of a Satoshi-style Merkle tree.

- #79 [Stratum] Use Merkle branches for fast recalculation of stratum work
  units.

  Customizing a block template for a particular miner involves changing the
  coinbase outputs which causes the segwit commitment to be updated.  In prior
  releases the stratum server made a copy of the entire block to perform this
  modification, which would be an expensive operation for large blocks.  This
  release changes that behavior to use pre-calculated Merkle branches for the
  coinbase and the block-final transaction (making use of the new stable
  Merkle branch APIs), which provides a large reduction in the cost of
  updating the block template for large blocks.

- #80 [Stratum] Nest initial response from stratum server.

  According to the stratum protocol specification, the first parameter in the
  response to `mining.subscribe` is supposed to be an array of messages, with
  each message itself being a parameter array.  So it is a list-of-lists,
  which is not what the prior behavior was.  This patch fixes a bug in the
  initial stratum server implementation.

- #81 [Stratum] Return "mining.set_difficulty" on first connection.

  Some mining proxies (e.g. Nicehash) reject connections that don't send a
  reasonable difficulty on first connection.  The actual value will be
  overridden when the miner is authorized and work is delivered.

- #82 [Stratum] Replace per-job nonce with extranonce1

  Some services (e.g. Nicehash) require the use of a non-empty string for
  extranonce1.  Since this value serves the same purpose as the work
  separating job nonce, we remove the prior job nonce value and use an
  equivalent 8-byte extranonce1 instead.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
