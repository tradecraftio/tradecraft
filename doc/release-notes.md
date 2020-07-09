Freicoin version v13.2.3-11817 is now available from:

  * [Linux i686 (Intel 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-i686-pc-linux-gnu.tar.gz)
  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-aarch64-linux-gnu.tar.gz)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.3-11817-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v13.2.3-11817.zip)

This is a new patch release, containing an emergency fix to the stratum server
interface to fix a bug which resulted in incorrect work being served to miners.
Upgrading is only required for nodes which make use of the stratum mining
interface.

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

This emergency release fixes a critical error in the stratum mining API's work
generation and share verification code which resulted in incorrect work being
served to miners in v13.2.2-11812 whenever the number of transactions in a block
was not expressible as 2^n or 2^n + 2^(n-1) for some integer n.

Since v13.2.2, the stratum mining code does a trick to avoid copying blocks when
verifying shares: it uses the so-called "stable" Merkle branch API to recompute
the right-half of the transaction Merkle tree when the segwit commitment is
updated.  The performance trick is that it pops off the top hash (representing
the left-hand side of the tree) of the block-final branch proof, so that the
recomputed "root" can be used to replace the last hash of the coinbase branch
proof, which is the topmost right-branch of the entire tree.  However prior to
this fix, any duplicated hashes between the last "real" hash value of the right
subtree and its root were not performed.  This resulted in an incorrect value
being used, and therefore the incorrect coinbase Merkle branch being reported to
miners.

v13.2.3-11817 Change log
========================

The following pull requests were merged into the Tradecraft code repository
for this release.

- #83 [Merkle] Perform excess repeated hashing on shortened "stable" Merkle
  roots.

  This is an update to the so-called "stable" Merkle branch code, but it fixes a
  bug in the stratum server API which shows up on witness blocks with a
  transaction count that is not 2^n or 2^n + 2^(n-1).

  Since v13.2.2, the stratum mining code does a trick to avoid copying blocks
  when verifying shares: it uses the stable Merkle branch code to recompute the
  right-half of the transaction Merkle tree when the segwit commitment is
  updated.  The trick is that it pops off the top hash (representing the
  left-hand side of the tree), so that the recomputed "root" is actually the
  root of the right-side subtree, which is the last hash of the coinbase branch
  proof.  However prior to this fix, any duplicated hashes between the last
  "real" hash value of the right subtree and its root were not performed.  This
  resulted in an incorrect value being used, and therefore the incorrect
  coinbase Merkle branch being reported to miners.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
