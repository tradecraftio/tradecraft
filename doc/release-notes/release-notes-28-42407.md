v28-42407 Release Notes
=======================

Freicoin version v28-42407 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v28-42407

This release includes new features, various bug fixes and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

How to Upgrade
--------------

If you are running an older version, shut it down.  Wait until it has completely shut down (which might take a few minutes in some cases), then run the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on macOS) or `freicoind`/`Freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is possible, but it might take some time if the data directory needs to be migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
-------------

Freicoin is supported and extensively tested on operating systems using the Linux Kernel 3.17+, macOS 11.0+, and Windows 7 and newer.  Freicoin should also work on most other UNIX-like systems but is not as frequently tested on them.  It is not recommended to use Freicoin on unsupported systems.

Notable changes
---------------

### Windows Data Directory

The default data directory on Windows has been moved from `C:\Users\Username\AppData\Roaming\Freicoin` to `C:\Users\Username\AppData\Local\Freicoin`.  Freicoin will check the existence of the old directory first and continue to use that directory for backwards compatibility if it is present.  (bitcoin/bitcoin#27064)

### JSON-RPC 2.0 Support

The JSON-RPC server now recognizes JSON-RPC 2.0 requests and responds with strict adherence to the [specification](https://www.jsonrpc.org/specification).  See [JSON-RPC-interface.md](https://github.com/tradecraftio/tradecraft/blob/master/doc/JSON-RPC-interface.md#json-rpc-11-vs-20) for details.  (bitcoin/bitcoin#27101)

JSON-RPC clients may need to be updated to be compatible with the JSON-RPC server.  Please open an issue on GitHub if any compatibility issues are found.

### libfreicoinconsensus Removal

The libfreicoin-consensus library was deprecated in v27-40537 and is now completely removed.  (bitcoin/bitcoin#29648)

### P2P and Network Changes

- Previously if Freicoin was listening for P2P connections, either using default settings or via `bind=addr:port` it would always also bind to `127.0.0.1:8640` to listen for Tor connections.  It was not possible to switch this off, even if the node didn't use Tor.  This has been changed and now `bind=addr:port` results in binding on `addr:port` only.  The default behavior of binding to `0.0.0.0:8639` and `127.0.0.1:8640` has not been changed.

  If you are using a `bind=...` configuration without `bind=...=onion` and rely on the previous implied behavior to accept incoming Tor connections at `127.0.0.1:8640`, you need to now make this explicit by using `bind=... bind=127.0.0.1:8640=onion`.  (bitcoin/bitcoin#22729)

- Freicoin will now fail to start up if any of its P2P binds fail, rather than the previous behaviour where it would only abort startup if all P2P binds had failed.  (bitcoin/bitcoin#22729)

- UNIX domain sockets can now be used for proxy connections.  Set `-onion` or `-proxy` to the local socket path with the prefix `unix:` (e.g. `-onion=unix:/home/me/torsocket`).  (bitcoin/bitcoin#27375)

- UNIX socket paths are now accepted for `-zmqpubrawblock` and `-zmqpubrawtx` with the format `-zmqpubrawtx=unix:/path/to/file`.  (bitcoin/bitcoin#27679)

- Additional "in" and "out" flags have been added to `-whitelist` to control whether permissions apply to inbound connections and/or manual ones (default: inbound only).  (bitcoin/bitcoin#27114)

- Transactions having a feerate that is too low will be opportunistically paired with their child transactions and submitted as a package, thus enabling the node to download 1-parent-1-child packages using the existing transaction relay protocol.  Combined with other mempool policies, this change allows limited "package relay" when a parent transaction is below the mempool minimum feerate.  Topologically Restricted Until Confirmation (TRUC) parents are additionally allowed to be below the minimum relay feerate (i.e., pay 0 fees).  Use the `submitpackage` RPC to submit packages directly to the node.  Warning: this P2P feature is limited (unlike the `submitpackage` interface, a child with multiple unconfirmed parents is not supported) and not yet reliable under adversarial conditions.  (bitcoin/bitcoin#28970)

### Mempool Policy Changes

- Transactions with version number set to 3 are now treated as standard on all networks (bitcoin/bitcoin#29496), subject to opt-in Topologically Restricted Until Confirmation (TRUC) transaction policy as described in [BIP 431](https://github.com/bitcoin/bips/blob/master/bip-0431.mediawiki).  The policy includes limits on spending unconfirmed outputs (bitcoin/bitcoin#28948), eviction of a previous descendant if a more incentive-compatible one is submitted (bitcoin/bitcoin#29306), and a maximum transaction size of 10,000vB (bitcoin/bitcoin#29873).  These restrictions simplify the assessment of incentive compatibility of accepting or replacing TRUC transactions, thus ensuring any replacements are more profitable for the node and making fee-bumping more reliable.

- Pay To Anchor (P2A) is a new standard witness output type for spending, a newly recognised output template.  This allows for key-less anchor outputs, with compact spending conditions for additional efficiencies on top of an equivalent `sh(OP_TRUE)` output, in addition to the txid stability of the spending transaction.  N.B. propagation of this output spending on the network will be limited until a sufficient number of nodes on the network adopt this upgrade.  (bitcoin/bitcoin#30352)

- Limited package RBF is now enabled, where the proposed conflicting package would result in a connected component, aka cluster, of size 2 in the mempool.  All clusters being conflicted against must be of size 2 or lower.  (bitcoin/bitcoin#28984)

### Updated RPCs

- The `dumptxoutset` RPC now returns the UTXO set dump in a new and improved format.  Correspondingly, the `loadtxoutset` RPC now expects this new format in the dumps it tries to load.  Dumps with the old format are no longer supported and need to be recreated using the new format to be usable.  (bitcoin/bitcoin#29612)

- AssumeUTXO mainnet parameters have been added for height 424,780.  This means the `loadtxoutset` RPC can now be used on mainnet with the matching UTXO set from that height.  (bitcoin/bitcoin#28553)

- The `warnings` field in `getblockchaininfo`, `getmininginfo` and `getnetworkinfo` now returns all the active node warnings as an array of strings, instead of a single warning.  The current behaviour can be temporarily restored by running Freicoin with the configuration option `-deprecatedrpc=warnings`.  (bitcoin/bitcoin#29845)

- Previously when using the `sendrawtransaction` RPC and specifying outputs that are already in the UTXO set, an RPC error code of `-27` with the message "Transaction already in block chain" was returned in response.  The error message has been changed to "Transaction outputs already in utxo set" to more accurately describe the source of the issue.  (bitcoin/bitcoin#30212)

- The default mode for the `estimatesmartfee` RPC has been updated from `conservative` to `economical`, which is expected to reduce over-estimation for many users, particularly if Replace-by-Fee is an option.  For users that require high confidence in their fee estimates at the cost of potentially over-estimating, the `conservative` mode remains available.  (bitcoin/bitcoin#30275)

- RPC `scantxoutset` now returns 2 new fields in the "unspents" JSON array: `blockhash` and `confirmations`.  See the scantxoutset help for details.  (bitcoin/bitcoin#30515)

- RPC `submitpackage` now allows 2 new arguments to be passed: `maxfeerate` and `maxburnamount`. See the subtmitpackage help for details.  (bitcoin/bitcoin#28950)

Changes to wallet-related RPCs can be found in the Wallet section below.

### Updated REST APIs

- Parameter validation for `/rest/getutxos` has been improved by rejecting truncated or overly large txids and malformed outpoint indices via raising an HTTP_BAD_REQUEST "Parse error".  These requests were previously handled silently.  (bitcoin/bitcoin#30482, bitcoin/bitcoin#30444)

### Build System

- GCC 11.1 or later, or Clang 16.0 or later, are now required to compile Freicoin.  (bitcoin/bitcoin#29091, bitcoin/bitcoin#30263)

- The minimum required glibc to run Freicoin is now 2.31.  This means that RHEL 8 and Ubuntu 18.04 (Bionic) are no-longer supported.  (bitcoin/bitcoin#29987)

- `--enable-lcov-branch-coverage` has been removed, given incompatibilities between lcov version 1 & 2.  `LCOV_OPTS` should be used to set any options instead.  (bitcoin/bitcoin#30192)

### Updated Settings

- When running with `-alertnotify`, an alert can now be raised multiple times instead of just once.  Previously, it was only raised when unknown new consensus rules were activated.  Its scope has now been increased to include all kernel warnings.  Specifically, alerts will now also be raised when an invalid chain with a large amount of work has been detected.  Additional warnings may be added in the future.  (bitcoin/bitcoin#30058)

Changes to GUI or wallet related settings can be found in the GUI or Wallet section below.

### Wallet

- The wallet now detects when wallet transactions conflict with the mempool.  Mempool-conflicting transactions can be seen in the `"mempoolconflicts"` field of `gettransaction`.  The inputs of mempool-conflicted transactions can now be respent without manually abandoning the transactions when the parent transaction is dropped from the mempool, which can cause wallet balances to appear higher.  (bitcoin/bitcoin#27307)

- A new `max_tx_weight` option has been added to the RPCs `fundrawtransaction`, `walletcreatefundedpsbt`, and `send`.  It specifies the maximum transaction weight.  If the limit is exceeded during funding, the transaction will not be built.  The default value is 4,000,000 WU.  (bitcoin/bitcoin#29523)

- A new `createwalletdescriptor` RPC allows users to add new automatically generated descriptors to their wallet.  This can be used to upgrade wallets created prior to the introduction of a new standard descriptor, such as taproot.  (bitcoin/bitcoin#29130)

- A new RPC `gethdkeys` lists all of the BIP32 HD keys in use by all of the descriptors in the wallet.  These keys can be used in conjunction with `createwalletdescriptor` to create and add single key descriptors to the wallet for a particular key that the wallet already knows.  (bitcoin/bitcoin#29130)

- The `sendall` RPC can now spend unconfirmed change and will include additional fees as necessary for the resulting transaction to bump the unconfirmed transactions' feerates to the specified feerate.  (bitcoin/bitcoin#28979)

- In RPC `bumpfee`, if a `fee_rate` is specified, the feerate is no longer restricted to following the wallet's incremental feerate of 5 sat/vb.  The feerate must still be at least the sum of the original fee and the mempool's incremental feerate.  (bitcoin/bitcoin#27969)

GUI Changes
-----------

- The "Migrate Wallet" menu allows users to migrate any legacy wallet in their wallet directory, regardless of the wallets loaded.  (bitcoin-core/gui#824)

- The "Information" window now displays the maximum mempool size along with the mempool usage.  (bitcoin-core/gui#825)

Low-level Changes
-----------------

### Tests

- A new `-testdatadir` option has been added to `test_freicoin` to allow specifying the location of unit test data directories.  (bitcoin/bitcoin#26564)

### Blockstorage

- Block files are now XOR'd by default with a key stored in the blocksdir.  Previous releases of Freicoin or previous external software will not be able to read the blocksdir with a non-zero XOR-key.  Refer to the `-blocksxor` help for more details.  (bitcoin/bitcoin#28052)

### Chainstate

- The chainstate database flushes that occur when blocks are pruned will no longer empty the database cache.  The cache will remain populated longer, which significantly reduces the time for initial block download to complete.  (bitcoin/bitcoin#28280)

### Dependencies

- The dependency on Boost.Process has been replaced with cpp-subprocess, which is contained in source.  Builders will no longer need Boost.Process to build with external signer support.  (bitcoin/bitcoin#28981)

Credits
-------

Thanks to everyone who directly contributed to this release:

- 0xb10c
- Alfonso Roman Zubeldia
- Andrew Toth
- AngusP
- Anthony Towns
- Antoine Poinsot
- Anton A
- Ava Chow
- Ayush Singh
- Ben Westgate
- Brandon Odiwuor
- brunoerg
- bstin
- Charlie
- Christopher Bergqvist
- Cory Fields
- crazeteam
- Daniela Brozzoni
- David Gumberg
- dergoegge
- Edil Medeiros
- Epic Curious
- Fabian Jahr
- fanquake
- furszy
- glozow
- Greg Sanders
- hanmz
- Hennadii Stepanov
- Hernan Marino
- Hodlinator
- ishaanam
- ismaelsadeeq
- Jadi
- Jon Atack
- josibake
- jrakibi
- kevkevin
- kevkevinpal
- Konstantin Akimov
- laanwj
- Larry Ruane
- Luis Schwab
- Luke Dashjr
- Lőrinc
- MarcoFalke
- marcofleon
- Mark Friedenbach
- Marnix
- Martin Saposnic
- Martin Zumsande
- Matt Corallo
- Matt Whitlock
- Matthew Zipkin
- Max Edwards
- Michael Dietz
- Murch
- nanlour
- pablomartin4btc
- Peter Todd
- Pieter Wuille
- @RandyMcMillan
- RoboSchmied
- Roman Zeyde
- Ryan Ofsky
- Sebastian Falbesoner
- Sergi Delgado Segura
- Sjors Provoost
- spicyzboss
- StevenMia
- stickies-v
- stratospher
- Suhas Daftuar
- sunerok
- tdb3
- TheCharlatan
- umiumi
- Vasil Dimov
- virtu
- willcl-ark

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
