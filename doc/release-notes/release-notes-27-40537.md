v27-40537 Release Notes
=======================

Freicoin version v27-40537 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v27-40537

This release includes new features, various bug fixes and performance improvements, as well as updated translations.

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

### libfreicoinconsensus

- libfreicoinconsensus is deprecated and will be removed for v28.  This library has existed for nearly 10 years with very little known uptake or impact.  It has become a maintenance burden.

  The underlying functionality does not change between versions, so any users of the library can continue to use the final release indefinitely, with the understanding that merge mining is its final consensus update.

  In the future, libfreicoinkernel will provide a much more useful API that is aware of the UTXO set, and therefore be able to fully validate transactions and blocks.  (bitcoin/bitcoin#29189)

### mempool.dat compatibility

- The `mempool.dat` file created by -persistmempool or the savemempool RPC will be written in a new format.  This new format includes the XOR'ing of transaction contents to mitigate issues where external programs (such as anti-virus) attempt to interpret and potentially modify the file.

  This new format can not be read by previous software releases. To allow for a downgrade, a temporary setting `-persistmempoolv1` has been added to fall back to the legacy format.  (bitcoin/bitcoin#28207)

### P2P and network changes

- Support for serving compatibility (non-auxpow) blocks and headers to peers older than v13.2.4-11864 has been removed.  Due to the difficulty transition, any peer which has not upgraded to a merge-mining aware release of Freicoin (v13.2.4-11864 or later) is vulnerable to hashrate-based attacks against its view of the blockchain.  In addition, the protocol cleanup hard-fork is scheduled to activate later this year, which will permanently fork older clients off the network anyway.

  If you are running an older version of Freicoin, it is recommended that upgrade immediately.  If for some reason you must keep the older version running, it is recommended that you firewall it by having it only connect to a v26.1-39475 node, which still supports serving non-auxpow block headers to unupgraded network peers.

- BIP324 v2 transport is now enabled by default. It remains possible to disable v2 by running with `-v2transport=0`.  (bitcoin/bitcoin#29347)

- Manual connection options (`-connect`, `-addnode` and `-seednode`) will now follow `-v2transport` to connect with v2 by default.  They will retry with v1 on failure.  (bitcoin/bitcoin#29058)

- Network-adjusted time has been removed from consensus code.  It is replaced with (unadjusted) system time. The warning for a large median time offset (70 minutes or more) is kept.  This removes the implicit security assumption of requiring an honest majority of outbound peers, and increases the importance of the node operator ensuring their system time is (and stays) correct to not fall out of consensus with the network.  (bitcoin/bitcoin#28956)

### Mempool Policy Changes

- Opt-in Topologically Restricted Until Confirmation (TRUC) Transactions policy (aka v3 transaction policy) is available for use on test networks when `-acceptnonstdtxn=1` is set.  By setting the transaction version number to 3, TRUC transactions request the application of limits on spending of their unconfirmed outputs.  These restrictions simplify the assessment of incentive compatibility of accepting or replacing TRUC transactions, thus ensuring any replacements are more profitable for the node and making fee-bumping more reliable.  TRUC transactions are currently nonstandard and can only be used on test networks where the standardness rules are relaxed or disabled (e.g. with `-acceptnonstdtxn=1`).  (bitcoin/bitcoin#28948)

### External Signing

- Support for external signing on Windows has been disabled.  It will be re-enabled once the underlying dependency (Boost Process), has been replaced with a different library.  (bitcoin/bitcoin#28967)

### Updated RPCs

- The addnode RPC now follows the `-v2transport` option (now on by default, see above) for making connections.  It remains possible to specify the transport type manually with the v2transport argument of addnode.  (bitcoin/bitcoin#29239)

### Build System

- A C++20 capable compiler is now required to build Freicoin.  (bitcoin/bitcoin#28349)

- macOS releases are configured to use the hardened runtime libraries.  (bitcoin/bitcoin#29127)

### Wallet

- The CoinGrinder coin selection algorithm has been introduced to mitigate unnecessary large input sets and lower transaction costs at high feerates.  CoinGrinder searches for the input set with minimal weight. Solutions found by CoinGrinder will produce a change output.  CoinGrinder is only active at elevated feerates (default: 30+ kria/vB, based on `-consolidatefeerate`×3).  (bitcoin/bitcoin#27877)

- The Branch And Bound coin selection algorithm will be disabled when the subtract fee from outputs feature is used.  (bitcoin/bitcoin#28994)

- If the birth time of a descriptor is detected to be later than the first transaction involving that descriptor, the birth time will be reset to the earlier time.  (bitcoin/bitcoin#28920)

Low-level changes
-----------------

### Pruning

- When pruning during initial block download, more blocks will be pruned at each flush in order to speed up the syncing of such nodes.  (bitcoin/bitcoin#20827)

### Init

- Various fixes to prevent issues where subsequent instances of Freicoin would result in deletion of files in use by an existing instance.  (bitcoin/bitcoin#28784, bitcoin/bitcoin#28946)

- Improved handling of empty `settings.json` files.  (bitcoin/bitcoin#29144)

Credits
-------

Thanks to everyone who directly contributed to this release:

- 22388o⚡️
- Aaron Clauson
- Amiti Uttarwar
- Andrew Toth
- Anthony Towns
- Antoine Poinsot
- Ava Chow
- Brandon Odiwuor
- brunoerg
- Chris Stewart
- Cory Fields
- dergoegge
- djschnei21
- Fabian Jahr
- fanquake
- furszy
- Gloria Zhao
- Greg Sanders
- Hennadii Stepanov
- Hernan Marino
- iamcarlos94
- ismaelsadeeq
- Jameson Lopp
- Jesse Barton
- John Moffett
- Jon Atack
- josibake
- jrakibi
- Justin Dhillon
- Kashif Smith
- kevkevin
- Kristaps Kaupe
- L0la L33tz
- Luke Dashjr
- Lőrinc
- marco
- MarcoFalke
- Mark Friedenbach
- Marnix
- Martin Leitner-Ankerl
- Martin Zumsande
- Max Edwards
- Murch
- muxator
- naiyoma
- Nikodemas Tuckus
- ns-xvrn
- pablomartin4btc
- Peter Todd
- Pieter Wuille
- Richard Myers
- Roman Zeyde
- Russell Yanofsky
- Ryan Ofsky
- Sebastian Falbesoner
- Sergi Delgado Segura
- Sjors Provoost
- stickies-v
- stratospher
- Supachai Kheawjuy
- TheCharlatan
- UdjinM6
- Vasil Dimov
- w0xlt
- willcl-ark

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
