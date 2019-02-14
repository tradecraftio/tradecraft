25.1-37601 Release Notes
========================

Freicoin version v25.1-37601 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v25.1-37601

This release includes various bug fixes and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes in some cases), then run the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on macOS) or `freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is possible, but it might take some time if the data directory needs to be migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
-------------

Freicoin is supported and extensively tested on operating systems using the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Freicoin should also work on most other Unix-like systems but is not as frequently tested on them.  It is not recommended to use Freicoin on unsupported systems.

Notable changes
---------------

### P2P

- bitcoin/bitcoin#27626 Parallel compact block downloads, take 3
- bitcoin/bitcoin#27743 p2p: Unconditionally return when compact block status == READ_STATUS_FAILED

### Fees

- bitcoin/bitcoin#27622 Fee estimation: avoid serving stale fee estimate

### RPC

- bitcoin/bitcoin#27727 rpc: Fix invalid bech32 address handling

### Rest

- bitcoin/bitcoin#27853 rest: fix crash error when calling /deploymentinfo
- bitcoin/bitcoin#28551 http: bugfix: allow server shutdown in case of remote client disconnection

### Wallet

- bitcoin/bitcoin#28038 wallet: address book migration bug fixes
- bitcoin/bitcoin#28067 descriptors: do not return top-level only funcs as sub descriptors
- bitcoin/bitcoin#28125 wallet: bugfix, disallow migration of invalid scripts
- bitcoin/bitcoin#28542 wallet: Check for uninitialized last processed and conflicting heights in MarkConflicted

### Build

- bitcoin/bitcoin#27724 build: disable boost multi index safe mode in debug mode
- bitcoin/bitcoin#28097 depends: xcb-proto 1.15.2
- bitcoin/bitcoin#28543 build, macos: Fix qt package build with new Xcode 15 linker
- bitcoin/bitcoin#28571 depends: fix unusable memory_resource in macos qt build

### Gui

- bitcoin-core/gui#751 macOS, do not process actions during shutdown

### Miscellaneous

- bitcoin/bitcoin#28452 Do not use std::vector = {} to release memory

### CI

- bitcoin/bitcoin#27777 ci: Prune dangling images on RESTART_CI_DOCKER_BEFORE_RUN
- bitcoin/bitcoin#27834 ci: Nuke Android APK task, Use credits for tsan
- bitcoin/bitcoin#27844 ci: Use podman stop over podman kill
- bitcoin/bitcoin#27886 ci: Switch to amd64 container in "ARM" task

Credits
-------

Thanks to everyone who directly contributed to this release:

- Abubakar Sadiq Ismail
- Andrew Chow
- Bruno Garcia
- Gregory Sanders
- Hennadii Stepanov
- MacroFake
- Mark Friedenbach
- Matias Furszyfer
- Michael Ford
- Pieter Wuille
- stickies-v
- Will Clark

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
