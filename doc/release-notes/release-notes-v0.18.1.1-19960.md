Bitcoin Core version v0.18.1.1-19960 is now available from:

  * [Linux i686 (Intel 32-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-i686-pc-linux-gnu.tar.gz)
  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-aarch64-linux-gnu.tar.gz)
  * [Linux RISC-V 64-bit (RV64GC)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-riscv64-linux-gnu.tar.gz)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.18.1.1-19960-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/bitcoin-v0.18.1.1-19960.zip)

This is the first release of the v0.18 stable branch of Bitcoin Core with the
stratum mining server and Tradecraft/Freicoin merge-mining patches applied.

Please report bugs related to the stratum mining server implementation or
Tradecraft/Freicoin merge-mining at the Tradecraft issue tracker on GitHub:

  <https://github.com/tradecraftio/tradecraft/issues>

Please report other bugs using Bitcoin the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications for the stratum mining server and
merge-mining patches, please subscribe to:

  <https://tradecraft.groups.io/g/announce/>

To receive security notifications for Bitcoin Core, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac) or
`bitcoind`/`bitcoin-qt` (on Linux).

The first time you run version 0.15.0 or newer, your chainstate database will be
converted to a new format, which will take anywhere from a few minutes to half
an hour, depending on the speed of your machine.

Note that the block database format also changed in version 0.8.0 and there is
no automatic upgrade code from before version 0.8 to version 0.15.0 or
later. Upgrading directly from 0.7.x and earlier without redownloading the
blockchain is not supported.  However, as usual, old wallet versions are still
supported.

Compatibility
=============

Bitcoin Core is supported and extensively tested on operating systems using the
Linux kernel, macOS 10.10+, and Windows 7 and newer. It is not recommended to
use Bitcoin Core on unsupported systems.

Bitcoin Core should also work on most other Unix-like systems but is not as
frequently tested on them.

From 0.17.0 onwards, macOS <10.10 is no longer supported. 0.17.0 is built using
Qt 5.9.x, which doesn't support versions of macOS older than 10.10.
Additionally, Bitcoin Core does not yet change appearance when macOS "dark mode"
is activated.

Known issues
============

Wallet GUI
----------

For advanced users who have both (1) enabled coin control features, and (2) are
using multiple wallets loaded at the same time: the coin control input selection
dialog can erroneously retain wrong-wallet state when switching wallets using
the dropdown menu.  For now, it is recommended not to use coin control features
with multiple wallets loaded.

Notable changes
===============

There have been no notable changes in the stratum mining server or the
Tradecraft/Freicoin merge-mining patches between v0.17.2.1-18199 and this
release.  Please see the Bitcoin Core release notes for notable changes between
the upstream versions upon which these releases are based.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
