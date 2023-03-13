v23.1.1-33175 Release Notes
===========================

Freicoin version v23.1.1-33175 is now available from:

  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-aarch64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, big endian)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-powerpc64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, little endian)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-powerpc64le-linux-gnu.tar.gz)
  * [Linux RISC-V 64-bit (RV64GC)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-riscv64-linux-gnu.tar.gz)
  * [macOS (Apple Silicon, app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-arm64-apple-darwin.dmg)
  * [macOS (Apple Silicon, server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-arm64-apple-darwin.tar.gz)
  * [macOS (Intel, app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-x86_64-apple-darwin.dmg)
  * [macOS (Intel, server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-x86_64-apple-darwin.tar.gz)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1.1-33175-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v23.1.1-33175.zip)

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tradecraftio/tradecraft/issues>

To receive security and update notifications, please subscribe to:

  <https://tradecraft.groups.io/g/announce/>

How to Upgrade
==============

If you are running an older version, shut it down.  Wait until it has
completely shut down (which might take a few minutes in some cases), then run
the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on
Mac) or `freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
=============

Freicoin is supported and extensively tested on operating systems using the
Linux kernel, macOS 10.15+, and Windows 7 and newer.  Freicoin should also
work on most other Unix-like systems but is not as frequently tested on them.
It is not recommended to use Freicoin on unsupported systems.

Notable changes
===============

v23.1.1-33175 contains some new features, bug fixes, and performance
improvements specific to the stratum mining server and Tradecraft/Freicoin
merge-mining patches.  Specifically, v24.0.1.2-35544 adds one major
improvements to the stratum mining server:

Default authorization credentials
---------------------------------

Users of the stratum mining server provide a Tradecraft/Freicoin address to
receive the block reward. Previously the Tradecraft/Freicoin address as
username was required.  With this release, the stratum mining server now
supports authorization defaults.

If the stratum miner does not provide a username (or the username field is
empty), then the configuration option `-defaultminingaddress` is used as the
Tradecraft/Freicoin address for that user.  If this configuration option is
not set, then the option `-stratumwallet` specifies the name of a wallet from
which to generate an address (or if given a non-zero value that does not
correspond to a wallet, then the default/first wallet is used).  When used in
this mode, a new address is generated for each found block.

When used as a merge-mining auxiliary work server, the stratum server reports
that an empty username can be used for authentication as a default credential.
If the merge-mining coordination server supports default authentication
credentials, then merge-mining will be performed with the default credentials
when the miner does not provide their own.  Note that the merge-mining
coordination server might override these defaults with its own
`-mergeminedefault` setting.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
