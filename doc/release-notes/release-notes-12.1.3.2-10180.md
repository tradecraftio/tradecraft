Freicoin version 12.1.3.2-10180 is now available from:

  * [Linux 32-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.2-10180-linux32.zip)
  * [Linux 64-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.2-10180-linux64.zip)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.2-10180-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.2-10180-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.2-10180-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.2-10180-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.2-10180-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.2-10180-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v12.1.3.2-10180.zip)

This is a new patch release, switching to a more conservative set of
activation rules for the block-final soft-fork, to prevent a possible
consensus partition.  Upgrading prior to activation of the block-final
soft-fork is recommended for mining nodes.

Please report bugs using the issue tracker at github:

  https://github.com/tradecraftio/tradecraft/issues

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has
completely shut down (which might take a few minutes for older
versions), then run the installer (on Windows) or just copy over
/Applications/Freicoin-Qt (on Mac) or freicoind/freicoin-qt (on
Linux).

Downgrade warning
-----------------

### Downgrade to a version < v12

Because release v12 and later will obfuscate the chainstate on every
fresh sync or reindex, the chainstate is not backwards-compatible with
pre-v12 versions of Freicoin or other software.

If you want to downgrade after you have done a reindex with v12 or
later, you will need to reindex when you first start Freicoin version
v11 or earlier.

This does not affect wallet forward or backward compatibility.

Notable changes
===============

Change in activation rules for block-final soft-fork
----------------------------------------------------

Some previously released builds of freicoind had activation rules that
would have prevented the chain from making progress after activation,
depending on the actions of the miner who found the activation block.
The issue is that those builds would refuse to validate the first
block after the maturation period if the original activation block
contained any non-trivial outputs in its coinbase.

In order to maximize compatibility and prevent a chain split, this
patch release updates the activation rules to require this activation
block to contain only trivially-spendable outputs, which is compatible
with all previously released versions of freicoind.

Miners should either upgrade to this patch release, or downgrade to
v12.1.2-10135 immediately.  Using a version prior to v12.1.2-10135 is
not safe to do since the locktime soft-fork achieved "locked-in"
status on block #256032.

12.1.3.2-10180 Change log
=========================

  * `add8ac24` [FinalTx]
    Require coinbase to have trivial-spend outputs at activation.

Credits
--------

Thanks to who contributed to this release, including:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
