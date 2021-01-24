v0.20.2.1-23672 Release Notes
=============================

Bitcoin Core version v0.20.2.1-23672 is now available from:

  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.20.2.1-23672-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.20.2.1-23672-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.20.2.1-23672-aarch64-linux-gnu.tar.gz)
  * [Linux RISC-V 64-bit (RV64GC)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.20.2.1-23672-riscv64-linux-gnu.tar.gz)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.20.2.1-23672-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.20.2.1-23672-osx64.tar.gz)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.20.2.1-23672-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.20.2.1-23672-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/bitcoin-v0.20.2.1-23672.zip)

This is the first release of the v0.20 stable branch of Bitcoin Core with the
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
shut down (which might take a few minutes in some cases), then run the installer
(on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac) or
`bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated. Old wallet versions of Bitcoin Core are generally supported.

Compatibility
=============

Bitcoin Core is supported and extensively tested on operating systems using the
Linux kernel, macOS 10.12+, and Windows 7 and newer.  Bitcoin Core should also
work on most other Unix-like systems but is not as frequently tested on them.
It is not recommended to use Bitcoin Core on unsupported systems.

From Bitcoin Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Bitcoin Core does not yet change appearance when
macOS "dark mode" is activated.

Known Bugs
==========

The process for generating the source code release ("tarball") has changed in an
effort to make it more complete, however, there are a few regressions in this
release:

- The generated `configure` script is currently missing, and you will need to
  install autotools and run `./autogen.sh` before you can run
  `./configure`. This is the same as when checking out from git.

- Instead of running `make` simply, you should instead run
  `BITCOIN_GENBUILD_NO_GIT=1 make`.

Notable changes
===============

There have been no notable changes in the stratum mining server or the
Tradecraft/Freicoin merge-mining patches between v0.19.2.1-21681 and this
release.  Please see the Bitcoin Core release notes for notable changes between
the upstream versions upon which these releases are based.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
