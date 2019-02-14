v26.2-39501 Release Notes
=========================

Freicoin version v26.2-39501 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v26.2-39501

This release includes new features, various bug fixes and performance improvements, as well as updated translations.

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

Freicoin is supported and extensively tested on operating systems using the Linux kernel, macOS 11.0+, and Windows 7 and newer.  Freicoin should also work on most other Unix-like systems but is not as frequently tested on them.  It is not recommended to use Freicoin on unsupported systems.

Notable changes
---------------

### P2P and network changes

- bitcoin/bitcoin#29691: Change Luke Dashjr seed to dashjr-list-of-p2p-nodes.us
- bitcoin/bitcoin#30085: p2p: detect addnode cjdns peers in GetAddedNodeInfo()

### RPC

- bitcoin/bitcoin#29869: rpc, bugfix: Enforce maximum value for setmocktime
- bitcoin/bitcoin#28554: bugfix: throw an error if an invalid parameter is passed to getnetworkhashps RPC
- bitcoin/bitcoin#30094: rpc: move UniValue in blockToJSON
- bitcoin/bitcoin#29870: rpc: Reword SighashFromStr error message

### Build

- bitcoin/bitcoin#29747: depends: fix mingw-w64 Qt DEBUG=1 build
- bitcoin/bitcoin#29985: depends: Fix build of Qt for 32-bit platforms with recent glibc
- bitcoin/bitcoin#30151: depends: Fetch miniupnpc sources from an alternative website
- bitcoin/bitcoin#30283: upnp: fix build with miniupnpc 2.2.8

### Misc

- bitcoin/bitcoin#29776: ThreadSanitizer: Fix #29767
- bitcoin/bitcoin#29856: ci: Bump s390x to ubuntu:24.04
- bitcoin/bitcoin#29764: doc: Suggest installing dev packages for debian/ubuntu qt5 build
- bitcoin/bitcoin#30149: contrib: Renew Windows code signing certificate

Credits
-------

Thanks to everyone who directly contributed to this release:

- Antoine Poinsot
- Ava Chow
- Cory Fields
- dergoegge
- fanquake
- glozow
- Hennadii Stepanov
- Jameson Lopp
- jonatack
- laanwj
- Luke Dashjr
- MarcoFalke
- Mark Friedenbach
- nanlour
- willcl-ark

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
