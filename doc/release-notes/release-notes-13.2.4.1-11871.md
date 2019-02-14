Freicoin version v13.2.4.1-11871 is now available from:

  * [Linux i686 (Intel 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-i686-pc-linux-gnu.tar.gz)
  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-aarch64-linux-gnu.tar.gz)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v13.2.4.1-11871-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v13.2.4.1-11871.zip)

This is a new patch release, containing a critical fix to the auxiliary
proof-of-work (AKA "merge mining") disk storage to fix a bug which prevents
initial block download and other issues on some peers after activation.
Upgrading is required for any node currently running v13.2.1-11864.

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

The recently released v13.2.4-11864 features activation parameters for the
auxiliary proof-of-work (AKA "merge mining") soft-fork.  Please see the release
notes accompanying v13.2.4-11864 for more information.

Fixes initial block download after auxpow activation
----------------------------------------------------

A bug in the chain state code v13.2.4-11864 prevents the auxiliary proof-of-work
information from being saved in the chain state database.  Since the auxiliary
proof-of-work structures remain in the block files this does not cause any
noticeable issues on running nodes.  However since the peer-to-peer network code
fetches block headers information from the chain state database, the bug does
prevent v13.2.4-11864 peers from serving post-activation headers to other
auxpow-aware nodes.

This v13.2.4.1-11871 emergency patch release fixes this bug, restoring the
expected behavior for the peer-to-peer network code, as well any other follow-on
bugs that likely exist but remain unidentified.

Upgrading (or downgrading to a pre-v13.2.4 release) is highly recommended for
all node operators currently running v13.2.4-11864.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as everyone that helped translating on
[Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
