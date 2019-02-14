Freicoin version v14.3.0.1-13385 is now available from:

  * [Linux i686 (Intel 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-i686-pc-linux-gnu.tar.gz)
  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-aarch64-linux-gnu.tar.gz)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v14.3.0.1-13385-win64.zip)
  * [Source](https://codeload.github.com/tradecraftio/tradecraft/zip/refs/tags/v14.3.0.1-13385)

This is a new patch release, containing a critical fix to the p2p block
transmission code to fix a bug which prevents merge-mined blocks from being
communicated to older clients.  Upgrading is highly recommended for nodes
running v13.2.4-11864 or later releases.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tradecraftio/tradecraft/issues>

To receive security and update notifications, please subscribe to:

  <https://tradecraft.groups.io/g/announce/>

Compatibility
=============

Freicoin is extensively tested on multiple operating systems using the Linux
kernel, macOS 10.8+, and Windows Vista and later.

Microsoft ended support for Windows XP on [April 8th,
2014](https://www.microsoft.com/en-us/WindowsForBusiness/end-of-xp-support), No
attempt is made to prevent installing or running the software on Windows XP, you
can still do so at your own risk but be aware that there are known instabilities
and issues.  Please do not report issues about Windows XP to the issue tracker.

Freicoin should also work on most other Unix-like systems but is not frequently
tested on them.

Notable changes
===============

The v13.2.4-11864 release introduced the auxiliary proof-of-work (AKA "merge
mining" soft-fork, which has since activated on both testnet and mainnet.
Please see the release notes accompanying v13.2.4-11864 for more information.
Unbeknownst by anyone at the time, a bug in the serialization & p2p network code
prevented auxiliary proof-of-work data structures from being stripped from block
headers before being transmitted directly to non-segwit supporting clients (v12
or earlier).

Headers sent to more recent segwit-supporting nodes would be properly stripped
and those nodes could relay the blocks to older clients, so this bug went
unnoticed for some time.  Now that nodes supporting segwit but not auxiliary
proof-of-work are becoming more infrequent, it was observed that v12 and earlier
nodes are having trouble syncing from the network.

Although v12 and earlier nodes are not generally supported, this emergency patch
release nevertheless fixes this serialization bug in order to re-enable syncing
directly to these pre-segwit nodes.

Upgrading is highly recommended for all nodes currently running clients between
v13.2.4-11864 and v14.3-13381.

Known Bugs
==========

Starting with v14.3-13381, the approximate transaction fee shown in Freicoin-Qt
when using coin control and smart fee estimation does not reflect any change in
target from the smart fee slider.  It will only present an approximate fee
calculated using the default target.  The fee calculated using the correct
target is still applied to the transaction and shown in the final send
confirmation dialog.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Fredrik Bodin
- Mark Friedenbach

As well as everyone that helped translating on
[Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
