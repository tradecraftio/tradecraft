v27.2-40603 Release Notes
=========================

Freicoin version v27.2-40603 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v27.2-40603

This release includes various bug fixes and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

How to Upgrade
--------------

If you are running an older version, shut it down.  Wait until it has completely shut down (which might take a few minutes in some cases), then run the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on macOS) or `freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is possible, but it might take some time if the data directory needs to be migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
-------------

Freicoin is supported and extensively tested on operating systems using the Linux Kernel 3.17+, macOS 11.0+, and Windows 7 and newer.  Freicoin should also work on most other Unix-like systems but is not as frequently tested on them.  It is not recommended to use Freicoin on unsupported systems.

Notable changes
---------------

### P2P

- #bitcoin/bitcoin30394 net: fix race condition in self-connect detection

### Init

- #bitcoin/bitcoin30435 init: change shutdown order of load block thread and scheduler

### RPC

- #bitcoin/bitcoin30357 Fix cases of calls to FillPST errantly returning complete=true

### PSBT

- #bitcoin/bitcoin29855 pst: Check non witness utxo outpoint early

### Test

- #bitcoin/bitcoin30552 test: fix constructor of msg_tx

### Doc

- #bitcoin/bitcoin30504 doc: use proper doxygen formatting for CTxMemPool::cs

### Build

- #bitcoin/bitcoin30283 upnp: fix build with miniupnpc 2.2.8
- #bitcoin/bitcoin30633 Fixes for GCC 15 compatibility

### CI

- #bitcoin/bitcoin30193 ci: move ASan job to GitHub Actions from Cirrus CI
- #bitcoin/bitcoin30299 ci: remove unused bcc variable from workflow

Credits
-------

Thanks to everyone who directly contributed to this release:

- Ava Chow
- Cory Fields
- Mark Friedenbach
- Martin Zumsande
- Matt Whitlock
- Max Edwards
- Sebastian Falbesoner
- Vasil Dimov
- willcl-ark

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
