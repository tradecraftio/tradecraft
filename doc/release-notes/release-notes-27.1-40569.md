v27.1-40569 Release Notes
=========================

Freicoin version v27.1-40569 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v27.1-40569

This release includes various bug fixes and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

How to Upgrade
==============

If you are running an older version, shut it down.  Wait until it has completely shut down (which might take a few minutes in some cases), then run the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on macOS) or `freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is possible, but it might take some time if the data directory needs to be migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
=============

Freicoin is supported and extensively tested on operating systems using the Linux Kernel 3.17+, macOS 11.0+, and Windows 7 and newer.  Freicoin should also work on most other Unix-like systems but is not as frequently tested on them.  It is not recommended to use Freicoin on unsupported systems.

Notable changes
===============

### RPC

- #29869 rpc, bugfix: Enforce maximum value for setmocktime
- #29870 rpc: Reword SighashFromStr error message
- #30094 rpc: move UniValue in blockToJSON

### Index

- #29776 Fix #29767, set m_synced = true after Commit()

### Gui

- #gui812 Fix create unsigned transaction fee bump
- #gui813 Don't permit port in proxy IP option

### Test

- #29892 test: Fix failing univalue float test

### P2P

- #30085 p2p: detect addnode cjdns peers in GetAddedNodeInfo()

### Build

- #29747 depends: fix mingw-w64 Qt DEBUG=1 build
- #29859 build: Fix false positive CHECK_ATOMIC test
- #29985 depends: Fix build of Qt for 32-bit platforms with recent glibc
- #30097 crypto: disable asan for sha256_sse4 with clang and -O0
- #30151 depends: Fetch miniupnpc sources from an alternative website
- #30216 build: Fix building fuzz binary on on SunOS / illumos
- #30217 depends: Update Boost download link

### Doc

- #29934 doc: add LLVM instruction for macOS < 13

### CI

- #29856 ci: Bump s390x to ubuntu:24.04

Credits
=======

Thanks to everyone who directly contributed to this release:

- Antoine Poinsot
- Ava Chow
- Cory Fields
- dergoegge
- fanquake
- furszy
- Hennadii Stepanov
- Jon Atack
- laanwj
- Luke Dashjr
- MarcoFalke
- Mark Friedenbach
- nanlour
- Sjors Provoost
- willcl-ark

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
