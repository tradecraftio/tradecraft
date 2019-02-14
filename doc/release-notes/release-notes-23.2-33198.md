v23.2-33198 Release Notes
=========================

Freicoin version v23.2-33198 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v23.2-33198

This release includes various bug fixes and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes in some cases), then run the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on Mac) or `freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is possible, but it might take some time if the data directory needs to be migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
-------------

Freicoin is supported and extensively tested on operating systems using the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Freicoin should also work on most other Unix-like systems but is not as frequently tested on them.  It is not recommended to use Freicoin on unsupported systems.

Notable changes
---------------

### P2P and network changes

- bitcoin/bitcoin#26909 net: prevent peers.dat corruptions by only serializing once
- bitcoin/bitcoin#27608 p2p: Avoid prematurely clearing download state for other peers
- bitcoin/bitcoin#27610 Improve performance of p2p inv to send queues

### Build system

- bitcoin/bitcoin#25436 build: suppress array-bounds errors in libxkbcommon
- bitcoin/bitcoin#25763 bdb: disable Werror for format-security
- bitcoin/bitcoin#26944 depends: fix systemtap download URL
- bitcoin/bitcoin#27462 depends: fix compiling bdb with clang-16 on aarch64

### Miscellaneous

- bitcoin/bitcoin#25444 ci: macOS task imrovements
- bitcoin/bitcoin#26388 ci: Use macos-ventura-xcode:14.1 image for "macOS native" task
- bitcoin/bitcoin#26924 refactor: Add missing includes to fix gcc-13 compile error

Credits
-------

Thanks to everyone who directly contributed to this release:

- Anthony Towns
- Hennadii Stepanov
- MacroFake
- Mark Friedenbach
- Martin Zumsande
- Michael Ford
- Suhas Daftuar

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
