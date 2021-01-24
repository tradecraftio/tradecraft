v25.1.0.1-37413 Release Notes
=============================

Bitcoin Core version v25.1.0.1-37413 is now available from:

  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-aarch64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, big endian)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-powerpc64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, little endian)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-powerpc64le-linux-gnu.tar.gz)
  * [Linux RISC-V 64-bit (RV64GC)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-riscv64-linux-gnu.tar.gz)
  * [macOS (Apple Silicon, app)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-arm64-apple-darwin.dmg)
  * [macOS (Apple Silicon, server)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-arm64-apple-darwin.tar.gz)
  * [macOS (Intel, app)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-x86_64-apple-darwin.dmg)
  * [macOS (Intel, server)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-x86_64-apple-darwin.tar.gz)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v25.1.0.1-37413-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/bitcoin-v25.1.0.1-37413.zip)

This is a point release of the v25 stable branch of Bitcoin Core with the
stratum mining server and Tradecraft/Freicoin merge-mining patches applied.

Please report bugs related to the stratum mining server implementation or
Tradecraft/Freicoin merge-mining at the Tradecraft issue tracker on GitHub:

  <https://github.com/tradecraftio/tradecraft/issues>

Please report other bugs regarding Bitcoin Core at the issue tracker on GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications for the stratum mining server and
merge-mining patches, please subscribe to:

  <https://tradecraft.groups.io/g/announce/>

To receive security notifications for Bitcoin Core, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the installer
(on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS) or
`bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated.  Old wallet versions of Bitcoin Core are generally supported.

Compatibility
=============

Bitcoin Core is supported and extensively tested on operating systems using the
Linux kernel, macOS 10.15+, and Windows 7 and newer.  Bitcoin Core should also
work on most other Unix-like systems but is not as frequently tested on them.
It is not recommended to use Bitcoin Core on unsupported systems.

Notable changes
===============

There have been no notable changes in the stratum mining server or the
Tradecraft/Freicoin merge-mining patches between v25.0.0.1-37362 and this
release.  Please see the Bitcoin Core release notes for notable changes between
the upstream versions upon which these releases are based.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
