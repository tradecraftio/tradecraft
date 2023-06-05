v24.1.0.1-35601 Release Notes
=============================

Bitcoin Core version v24.1.0.1-35601 is now available from:

  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-aarch64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, big endian)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-powerpc64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, little endian)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-powerpc64le-linux-gnu.tar.gz)
  * [Linux RISC-V 64-bit (RV64GC)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-riscv64-linux-gnu.tar.gz)
  * [macOS (Apple Silicon, app)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-arm64-apple-darwin.dmg)
  * [macOS (Apple Silicon, server)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-arm64-apple-darwin.tar.gz)
  * [macOS (Intel, app)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-x86_64-apple-darwin.dmg)
  * [macOS (Intel, server)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-x86_64-apple-darwin.tar.gz)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.1.0.1-35601-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/bitcoin-v24.1.0.1-35601.zip)

This is a minor update of the v24 stable branch of Bitcoin Core with the
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

If you are running an older version, shut it down. Wait until it has
completely shut down (which might take a few minutes in some cases), then run
the installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on
macOS) or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated.  Old wallet versions of Bitcoin Core are generally supported.

Compatibility
=============

Bitcoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Bitcoin Core should
also work on most other Unix-like systems but is not as frequently tested on
them. It is not recommended to use Bitcoin Core on unsupported systems.

Notable changes
===============

There have been upstream changes between v24.0.1.2-35544 and this release.
Please see the Bitcoin Core release notes for notable changes between the
upstream versions upon which these releases are based.

In addition, the following changes have been made specifically to the
Tradecraft/Freicoin merge-mining patch set.

Fixed delay on shutdown due to wallet lock
------------------------------------------

A longstanding bug in the merge-mining patch set has been fixed that caused
the wallet to be remain deadlocked on shutdown, triggering a long timeout
delay before shutdown can complete.  Shutdown/restart of Bitcoin Core server
with stratum and merge-mining patches applied should now be much faster.

Switch to libevent for watchdog timer
-------------------------------------

The merge-mining patch set has been updated to use libevent for the watchdog
timer, instead of the custom timer implementation used previously.  This
allows processing to continue while the watchdog timer is running, thereby
preventing delays when new work arrives.

New RPC: getstratuminfo
-----------------------

A new RPC `getstratuminfo` has been added to the stratum mining server.  This
RPC returns information about the stratum mining server's configuration and
connected miners.

New RPC: getmergemineinfo
-------------------------

A new RPC `getmergemineinfo` has been added to the stratum mining server.
This RPC returns information about connected auxiliary work servers, and the
current state of the merge mining system.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
