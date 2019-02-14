v11.3-8698 Release Notes
========================

Freicoin version v11.3-8698 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v11.3-8698

This is a new major version release, bringing both new features and bug fixes.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes for older versions), then run the installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or bitcoind/bitcoin-qt (on Linux).

Downgrade warning
------------------

Because release 10.4 and later makes use of headers-first synchronization and parallel block download (see further), the block files and databases are not backwards-compatible with pre-10 versions of Freicoin or other software:

- Blocks will be stored on disk out of order (in the order they are received, really), which makes it incompatible with some tools or other programs. Reindexing using earlier versions will also not work anymore as a result of this.

- The block index database will now hold headers for which no block is stored on disk, which earlier versions won't support.

If you want to be able to downgrade smoothly, make a backup of your entire data directory. Without this your node will need start syncing (or importing from bootstrap.dat) anew afterwards. It is possible that the data from a completely synchronised 10.4 node may be usable in older versions as-is, but this is not supported and may break as soon as the older version attempts to reindex.

This does not affect wallet forward or backward compatibility. There are no known problems when downgrading from 11.x to 10.x.

Important information
---------------------

### Transaction flooding

At the time of this release, it is possible for the P2P network to be flooded with low-fee transactions. This would cause a ballooning of the mempool size.

If this happens and growth of the mempool causes problematic memory use on your node, it is possible to change a few configuration options to work around this. The growth of the mempool can be monitored with the RPC command `getmempoolinfo`.

One is to increase the minimum transaction relay fee `minrelaytxfee`, which defaults to 0.00005. This will cause transactions with fewer BTC/kB fee to be rejected, and thus fewer transactions entering the mempool.

The other is to restrict the relaying of free transactions with `limitfreerelay`. This option sets the number of kB/minute at which free transactions (with enough priority) will be accepted. It defaults to 15.  Reducing this number reduces the speed at which the mempool can grow due to free transactions.

For example, add the following to `freicoin.conf`:

    minrelaytxfee=0.00025 
    limitfreerelay=5

More robust solutions will arrive in a future release.

### BIP113 mempool-only locktime enforcement using GetMedianTimePast()

Freicoin transactions currently may specify a locktime indicating when they may be added to a valid block.  Current consensus rules require that blocks have a block header time greater than the locktime specified in any transaction in that block.

Miners get to choose what time they use for their header time, with the consensus rule being that no node will accept a block whose time is more than two hours in the future.  This creates an incentive for miners to set their header times to future values in order to include locktimed transactions which weren't supposed to be included for up to two more hours.

The consensus rules also specify that valid blocks may have a header time greater than that of the median of the 11 previous blocks.  This GetMedianTimePast() time has a key feature we generally associate with time: it can't go backwards.

[BIP113][] specifies a soft fork (**not enforced in this release**) that weakens this perverse incentive for individual miners to use a future time by requiring that valid blocks have a computed GetMedianTimePast() greater than the locktime specified in any transaction in that block.

Mempool inclusion rules currently require transactions to be valid for immediate inclusion in a block in order to be accepted into the mempool.  This release begins applying the BIP113 rule to received transactions, so transaction whose time is greater than the GetMedianTimePast() will no longer be accepted into the mempool.

**Implication for miners:** you will begin rejecting transactions that would not be valid under BIP113, which will prevent you from producing invalid blocks if/when BIP113 is enforced on the network. Any transactions which are valid under the current rules but not yet valid under the BIP113 rules will either be mined by other miners or delayed until they are valid under BIP113. Note, however, that time-based locktime transactions are more or less unseen on the network currently.

**Implication for users:** GetMedianTimePast() always trails behind the current time, so a transaction locktime set to the present time will be rejected by nodes running this release until the median time moves forward. To compensate, subtract one hour (3,600 seconds) from your locktimes to allow those transactions to be included in mempools at approximately the expected time.

[BIP113]: https://github.com/bitcoin/bips/blob/master/bip-0113.mediawiki

Notable changes
---------------

### Block file pruning

This release supports running a fully validating node without maintaining a copy of the raw block and undo data on disk. To recap, there are four types of data related to the blockchain in the freicoin system: the raw blocks as received over the network (blk???.dat), the undo data (rev???.dat), the block index and the UTXO set (both LevelDB databases). The databases are built from the raw data.

Block pruning allows Freicoin to delete the raw block and undo data once it's been validated and used to build the databases. At that point, the raw data is used only to relay blocks to other nodes, to handle reorganizations, to look up old transactions (if -txindex is enabled or via the RPC/REST interfaces), or for rescanning the wallet. The block index continues to hold the metadata about all blocks in the blockchain.

The user specifies how much space to allot for block & undo files. The minimum allowed is 550MB. Note that this is in addition to whatever is required for the block index and UTXO databases. The minimum was chosen so that Freicoin will be able to maintain at least 288 blocks on disk (two days worth of blocks at 10 minutes per block). In rare instances it is possible that the amount of space used will exceed the pruning target in order to keep the required last 288 blocks on disk.

Note that because the block file size is 2,032 MiB, and pruning occurs on full blocks only, the disk usage for this data must exceed 2,032 MiB for the block file plus the size of the corresponding undo file, plus the size of the last 288 blocks before any pruning is to occur.

Block pruning works during initial sync in the same way as during steady state, by deleting block files "as you go" whenever disk space is allocated. Thus, if the user specifies 550MB, once that level is reached the program will begin deleting the oldest block and undo files, while continuing to download the blockchain.

For now, block pruning disables block relay.  In the future, nodes with block pruning will at a minimum relay "new" blocks, meaning blocks that extend their active chain.

Block pruning is currently incompatible with running a wallet due to the fact that block data is used for rescanning the wallet and importing keys or addresses (which require a rescan.) However, running the wallet with block pruning will be supported in the near future, subject to those limitations.

Block pruning is also incompatible with -txindex and will automatically disable it.

Once you have pruned blocks, going back to unpruned state requires re-downloading the entire blockchain. To do this, re-start the node with -reindex. Note also that any problem that would cause a user to reindex (e.g., disk corruption) will cause a pruned node to redownload the entire blockchain.  Finally, note that when a pruned node reindexes, it will delete any blk???.dat and rev???.dat files in the data directory prior to restarting the download.

To enable block pruning on the command line:

- `-prune=N`: where N is the number of MB to allot for raw block & undo data.

Modified RPC calls:

- `getblockchaininfo` now includes whether we are in pruned mode or not.

- `getblock` will check if the block's data has been pruned and if so, return an error.

- `getrawtransaction` will no longer be able to locate a transaction that has a UTXO but where its block file has been pruned. 

Pruning is disabled by default.

### Big endian support

Experimental support for big-endian CPU architectures was added in this release. All little-endian specific code was replaced with endian-neutral constructs. This has been tested on at least MIPS and PPC hosts. The build system will automatically detect the endianness of the target.

### Memory usage optimization

There have been many changes in this release to reduce the default memory usage of a node, among which:

- Accurate UTXO cache size accounting (#6102); this makes the option `-dbcache` precise where this grossly underestimated memory usage before

- Reduce size of per-peer data structure (#6064 and others); this increases the number of connections that can be supported with the same amount of memory

- Reduce the number of threads (#5964, #5679); lowers the amount of (esp. virtual) memory needed

### Fee estimation changes

This release improves the algorithm used for fee estimation.  Previously, -1 was returned when there was insufficient data to give an estimate.  Now, -1 will also be returned when there is no fee or priority high enough for the desired confirmation target. In those cases, it can help to ask for an estimate for a higher target number of blocks. It is not uncommon for there to be no fee or priority high enough to be reliably (85%) included in the next block and for this reason, the default for `-txconfirmtarget=n` has changed from 1 to 2.

### Privacy: Disable wallet transaction broadcast

This release adds an option `-walletbroadcast=0` to prevent automatic transaction broadcast and rebroadcast (#5951). This option allows separating transaction submission from the node functionality.

Making use of this, third-party scripts can be written to take care of transaction (re)broadcast:

- Send the transaction as normal, either through RPC or the GUI

- Retrieve the transaction data through RPC using `gettransaction` (NOT `getrawtransaction`). The `hex` field of the result will contain the raw hexadecimal representation of the transaction

- The transaction can then be broadcasted through arbitrary mechanisms supported by the script

One such application is selective Tor usage, where the node runs on the normal internet but transactions are broadcasted over Tor.

For an example script see [bitcoin-submittx](https://github.com/laanwj/bitcoin-submittx).

### Privacy: Stream isolation for Tor

This release adds functionality to create a new circuit for every peer connection, when the software is used with Tor. The new option, `-proxyrandomize`, is on by default.

When enabled, every outgoing connection will (potentially) go through a different exit node. That significantly reduces the chance to get unlucky and pick a single exit node that is either malicious, or widely banned from the P2P network. This improves connection reliability as well as privacy, especially for the initial connections.

**Important note:** If a non-Tor SOCKS5 proxy is configured that supports authentication, but doesn't require it, this change may cause that proxy to reject connections. A user and password is sent where they weren't before. This setup is exceedingly rare, but in this case `-proxyrandomize=0` can be passed to disable the behavior.

v11.3-8698 Change log
---------------------

Detailed release notes follow. This overview includes changes that affect behavior, not code moves, refactors and string updates. For convenience in locating the code changes and accompanying discussion, both the bitcoin pull request and git merge commit are mentioned.

### RPC and REST
- bitcoin/bitcoin#5461 `5f7279a` signrawtransaction: validate private key
- bitcoin/bitcoin#5444 `103f66b` Add /rest/headers/<count>/<hash>.<ext>
- bitcoin/bitcoin#4964 `95ecc0a` Add scriptPubKey field to validateaddress RPC call
- bitcoin/bitcoin#5476 `c986972` Add time offset into getpeerinfo output
- bitcoin/bitcoin#5540 `84eba47` Add unconfirmed and immature balances to getwalletinfo
- bitcoin/bitcoin#5599 `40e96a3` Get rid of the internal miner's hashmeter
- bitcoin/bitcoin#5711 `87ecfb0` Push down RPC locks
- bitcoin/bitcoin#5754 `1c4e3f9` fix getblocktemplate lock issue
- bitcoin/bitcoin#5756 `5d901d8` Fix getblocktemplate_proposals test by mining one block
- bitcoin/bitcoin#5548 `d48ce48` Add /rest/chaininfos
- bitcoin/bitcoin#5992 `4c4f1b4` Push down RPC reqWallet flag
- bitcoin/bitcoin#6036 `585b5db` Show zero value txouts in listunspent
- bitcoin/bitcoin#5199 `6364408` Add RPC call `gettxoutproof` to generate and verify merkle blocks
- bitcoin/bitcoin#5418 `16341cc` Report missing inputs in sendrawtransaction
- bitcoin/bitcoin#5937 `40f5e8d` show script verification errors in signrawtransaction result
- bitcoin/bitcoin#5420 `1fd2d39` getutxos REST command (based on Bip64)
- bitcoin/bitcoin#6193 `42746b0` [REST] remove json input for getutxos, limit to query max. 15 outpoints
- bitcoin/bitcoin#6226 `5901596` json: fail read_string if string contains trailing garbage

### Configuration and command-line options
- bitcoin/bitcoin#5636 `a353ad4` Add option `-allowselfsignedrootcertificate` to allow self signed root certs (for testing payment requests)
- bitcoin/bitcoin#5900 `3e8a1f2` Add a consistency check `-checkblockindex` for the block chain data structures
- bitcoin/bitcoin#5951 `7efc9cf` Make it possible to disable wallet transaction broadcast (using `-walletbroadcast=0`)
- bitcoin/bitcoin#5911 `b6ea3bc` privacy: Stream isolation for Tor (on by default, use `-proxyrandomize=0` to disable)
- bitcoin/bitcoin#5863 `c271304` Add autoprune functionality (`-prune=<size>`)
- bitcoin/bitcoin#6153 `0bcf04f` Parameter interaction: disable upnp if -proxy set
- bitcoin/bitcoin#6274 `4d9c7fe` Add option `-alerts` to opt out of alert system

### Block and transaction handling
- bitcoin/bitcoin#5367 `dcc1304` Do all block index writes in a batch
- bitcoin/bitcoin#5253 `203632d` Check against MANDATORY flags prior to accepting to mempool
- bitcoin/bitcoin#5459 `4406c3e` Reject headers that build on an invalid parent
- bitcoin/bitcoin#5481 `055f3ae` Apply AreSane() checks to the fees from the network
- bitcoin/bitcoin#5580 `40d65eb` Preemptively catch a few potential bugs
- bitcoin/bitcoin#5349 `f55c5e9` Implement test for merkle tree malleability in CPartialMerkleTree
- bitcoin/bitcoin#5564 `a89b837` clarify obscure uses of EvalScript()
- bitcoin/bitcoin#5521 `8e4578a` Reject non-final txs even in testnet/regtest
- bitcoin/bitcoin#5707 `6af674e` Change hardcoded character constants to descriptive named constants for db keys
- bitcoin/bitcoin#5286 `fcf646c` Change the default maximum OP_RETURN size to 80 bytes
- bitcoin/bitcoin#5710 `175d86e` Add more information to errors in ReadBlockFromDisk
- bitcoin/bitcoin#5948 `b36f1ce` Use GetAncestor to compute new target
- bitcoin/bitcoin#5959 `a0bfc69` Add additional block index consistency checks
- bitcoin/bitcoin#6058 `7e0e7f8` autoprune minor post-merge improvements
- bitcoin/bitcoin#5159 `2cc1372` New fee estimation code
- bitcoin/bitcoin#6102 `6fb90d8` Implement accurate UTXO cache size accounting
- bitcoin/bitcoin#6129 `2a82298` Bug fix for clearing fCheckForPruning
- bitcoin/bitcoin#5947 `e9af4e6` Alert if it is very likely we are getting a bad chain
- bitcoin/bitcoin#6203 `c00ae64` Remove P2SH coinbase flag, no longer interesting
- bitcoin/bitcoin#5985 `37b4e42` Fix removing of orphan transactions
- bitcoin/bitcoin#6221 `6cb70ca` Prune: Support noncontiguous block files
- bitcoin/bitcoin#6256 `fce474c` Use best header chain timestamps to detect partitioning
- bitcoin/bitcoin#6233 `a587606` Advance pindexLastCommonBlock for blocks in chainActive

### P2P protocol and network code
- bitcoin/bitcoin#5507 `844ace9` Prevent DOS attacks on in-flight data structures
- bitcoin/bitcoin#5770 `32a8b6a` Sanitize command strings before logging them
- bitcoin/bitcoin#5859 `dd4ffce` Add correct bool combiner for net signals
- bitcoin/bitcoin#5876 `8e4fd0c` Add a NODE_GETUTXO service bit and document NODE_NETWORK
- bitcoin/bitcoin#6028 `b9311fb` Move nLastTry from CAddress to CAddrInfo
- bitcoin/bitcoin#5662 `5048465` Change download logic to allow calling getdata on inbound peers
- bitcoin/bitcoin#5971 `18d2832` replace absolute sleep with conditional wait
- bitcoin/bitcoin#5918 `7bf5d5e` Use equivalent PoW for non-main-chain requests
- bitcoin/bitcoin#6059 `f026ab6` chainparams: use SeedSpec6's rather than CAddress's for fixed seeds
- bitcoin/bitcoin#6080 `31c0bf1` Add jonasschnellis dns seeder
- bitcoin/bitcoin#5976 `9f7809f` Reduce download timeouts as blocks arrive
- bitcoin/bitcoin#6172 `b4bbad1` Ignore getheaders requests when not synced
- bitcoin/bitcoin#5875 `304892f` Be stricter in processing unrequested blocks
- bitcoin/bitcoin#6333 `41bbc85` Hardcoded seeds update June 2015

### Validation
- bitcoin/bitcoin#5143 `48e1765` Implement BIP62 rule 6
- bitcoin/bitcoin#5713 `41e6e4c` Implement BIP66

### Build system
- bitcoin/bitcoin#5501 `c76c9d2` Add mips, mipsel and aarch64 to depends platforms
- bitcoin/bitcoin#5334 `cf87536` libbitcoinconsensus: Add pkg-config support
- bitcoin/bitcoin#5514 `ed11d53` Fix 'make distcheck'
- bitcoin/bitcoin#5505 `a99ef7d` Build winshutdownmonitor.cpp on Windows only
- bitcoin/bitcoin#5582 `e8a6639` Osx toolchain update
- bitcoin/bitcoin#5684 `ab64022` osx: bump build sdk to 10.9
- bitcoin/bitcoin#5695 `23ef5b7` depends: latest config.guess and config.sub
- bitcoin/bitcoin#5509 `31dedb4` Fixes when compiling in c++11 mode
- bitcoin/bitcoin#5819 `f8e68f7` release: use static libstdc++ and disable reduced exports by default
- bitcoin/bitcoin#5510 `7c3fbc3` Big endian support
- bitcoin/bitcoin#5149 `c7abfa5` Add script to verify all merge commits are signed
- bitcoin/bitcoin#6082 `7abbb7e` qt: disable qt tests when one of the checks for the gui fails
- bitcoin/bitcoin#6244 `0401aa2` configure: Detect (and reject) LibreSSL
- bitcoin/bitcoin#6269 `95aca44` gitian: Use the new bitcoin-detached-sigs git repo for OSX signatures
- bitcoin/bitcoin#6285 `ef1d506` Fix scheduler build with some boost versions.
- bitcoin/bitcoin#6280 `25c2216` depends: fix Boost 1.55 build on GCC 5
- bitcoin/bitcoin#6303 `b711599` gitian: add a gitian-win-signer descriptor
- bitcoin/bitcoin#6246 `8ea6d37` Fix build on FreeBSD
- bitcoin/bitcoin#6282 `daf956b` fix crash on shutdown when e.g. changing -txindex and abort action
- bitcoin/bitcoin#6354 `bdf0d94` Gitian windows signing normalization

### Wallet
- #2340 `811c71d` Discourage fee sniping with nLockTime
- bitcoin/bitcoin#5485 `d01bcc4` Enforce minRelayTxFee on wallet created tx and add a maxtxfee option
- bitcoin/bitcoin#5508 `9a5cabf` Add RandAddSeedPerfmon to MakeNewKey
- bitcoin/bitcoin#4805 `8204e19` Do not flush the wallet in AddToWalletIfInvolvingMe(..)
- bitcoin/bitcoin#5319 `93b7544` Clean up wallet encryption code
- bitcoin/bitcoin#5831 `df5c246` Subtract fee from amount
- bitcoin/bitcoin#6076 `6c97fd1` wallet: fix boost::get usage with boost 1.58
- bitcoin/bitcoin#5511 `23c998d` Sort pending wallet transactions before reaccepting
- bitcoin/bitcoin#6126 `26e08a1` Change default nTxConfirmTarget to 2
- bitcoin/bitcoin#6183 `75a4d51` Fix off-by-one error w/ nLockTime in the wallet
- bitcoin/bitcoin#6276 `c9fd907` Fix getbalance * 0

### GUI
- bitcoin/bitcoin#5219 `f3af0c8` New icons
- bitcoin/bitcoin#5228 `bb3c75b` HiDPI (retina) support for splash screen
- bitcoin/bitcoin#5258 `73cbf0a` The RPC Console should be a QWidget to make window more independent
- bitcoin/bitcoin#5488 `851dfc7` Light blue icon color for regtest
- bitcoin/bitcoin#5547 `a39aa74` New icon for the debug window
- bitcoin/bitcoin#5493 `e515309` Adopt style colour for button icons
- bitcoin/bitcoin#5557 `70477a0` On close of splashscreen interrupt verifyDB
- bitcoin/bitcoin#5559 `83be8fd` Make the command-line-args dialog better
- bitcoin/bitcoin#5144 `c5380a9` Elaborate on signverify message dialog warning
- bitcoin/bitcoin#5489 `d1aa3c6` Optimize PNG files
- bitcoin/bitcoin#5649 `e0cd2f5` Use text-color icons for system tray Send/Receive menu entries
- bitcoin/bitcoin#5651 `848f55d` Coin Control: Use U+2248 "ALMOST EQUAL TO" rather than a simple tilde
- bitcoin/bitcoin#5626 `ab0d798` Fix icon sizes and column width
- bitcoin/bitcoin#5683 `c7b22aa` add new osx dmg background picture
- bitcoin/bitcoin#5620 `7823598` Payment request expiration bug fix
- bitcoin/bitcoin#5729 `9c4a5a5` Allow unit changes for read-only BitcoinAmountField
- bitcoin/bitcoin#5753 `0f44672` Add bitcoin logo to about screen
- bitcoin/bitcoin#5629 `a956586` Prevent amount overflow problem with payment requests
- bitcoin/bitcoin#5830 `215475a` Don't save geometry for options and about/help window
- bitcoin/bitcoin#5793 `d26f0b2` Honor current network when creating autostart link
- bitcoin/bitcoin#5847 `f238add` Startup script for centos, with documentation
- bitcoin/bitcoin#5915 `5bd3a92` Fix a static qt5 crash when using certain versions of libxcb
- bitcoin/bitcoin#5898 `bb56781` Fix rpc console font size to flexible metrics
- bitcoin/bitcoin#5467 `bc8535b` Payment request / server work - part 2
- bitcoin/bitcoin#6161 `180c164` Remove movable option for toolbar
- bitcoin/bitcoin#6160 `0d862c2` Overviewpage: make sure warning icons gets colored

### Tests
- bitcoin/bitcoin#5453 `2f2d337` Add ability to run single test manually to RPC tests
- bitcoin/bitcoin#5421 `886eb57` Test unexecuted OP_CODESEPARATOR
- bitcoin/bitcoin#5530 `565b300` Additional rpc tests
- bitcoin/bitcoin#5611 `37b185c` Fix spurious windows test failures after 012598880c
- bitcoin/bitcoin#5613 `2eda47b` Fix smartfees test for change to relay policy
- bitcoin/bitcoin#5612 `e3f5727` Fix zapwallettxes test
- bitcoin/bitcoin#5642 `30a5b5f` Prepare paymentservertests for new unit tests
- bitcoin/bitcoin#5784 `e3a3cd7` Fix usage of NegateSignatureS in script_tests
- bitcoin/bitcoin#5813 `ee9f2bf` Add unit tests for next difficulty calculations
- bitcoin/bitcoin#5855 `d7989c0` Travis: run unit tests in different orders
- bitcoin/bitcoin#5852 `cdae53e` Reinitialize state in between individual unit tests.
- bitcoin/bitcoin#5883 `164d7b6` tests: add a BasicTestingSetup and apply to all tests
- bitcoin/bitcoin#5940 `446bb70` Regression test for ResendWalletTransactions
- bitcoin/bitcoin#6052 `cf7adad` fix and enable bip32 unit test
- bitcoin/bitcoin#6039 `734f80a` tests: Error when setgenerate is used on regtest
- bitcoin/bitcoin#6074 `948beaf` Correct the PUSHDATA4 minimal encoding test in script_invalid.json
- bitcoin/bitcoin#6032 `e08886d` Stop nodes after RPC tests, even with --nocleanup
- bitcoin/bitcoin#6075 `df1609f` Add additional script edge condition tests
- bitcoin/bitcoin#5981 `da38dc6` Python P2P testing 
- bitcoin/bitcoin#5958 `9ef00c3` Add multisig rpc tests
- bitcoin/bitcoin#6112 `fec5c0e` Add more script edge condition tests

### Miscellaneous
- bitcoin/bitcoin#5457, #5506, #5952, #6047 Update libsecp256k1
- bitcoin/bitcoin#5437 `84857e8` Add missing CAutoFile::IsNull() check in main
- bitcoin/bitcoin#5490 `ec20fd7` Replace uint256/uint160 with opaque blobs where possible
- bitcoin/bitcoin#5654, #5764 Adding jonasschnelli's GPG key
- bitcoin/bitcoin#5477 `5f04d1d` OS X 10.10: LSSharedFileListItemResolve() is deprecated
- bitcoin/bitcoin#5679 `beff11a` Get rid of DetectShutdownThread
- bitcoin/bitcoin#5787 `9bd8c9b` Add fanquake PGP key
- bitcoin/bitcoin#5366 `47a79bb` No longer check osx compatibility in RenameThread
- bitcoin/bitcoin#5689 `07f4386` openssl: abstract out OPENSSL_cleanse
- bitcoin/bitcoin#5708 `8b298ca` Add list of implemented BIPs
- bitcoin/bitcoin#5809 `46bfbe7` Add bitcoin-cli man page
- bitcoin/bitcoin#5839 `86eb461` keys: remove libsecp256k1 verification until it's actually supported
- bitcoin/bitcoin#5749 `d734d87` Help messages correctly formatted (79 chars)
- bitcoin/bitcoin#5884 `7077fe6` BUGFIX: Stack around the variable 'rv' was corrupted
- bitcoin/bitcoin#5849 `41259ca` contrib/init/bitcoind.openrc: Compatibility with previous OpenRC init script variables
- bitcoin/bitcoin#5950 `41113e3` Fix locale fallback and guard tests against invalid locale settings
- bitcoin/bitcoin#5965 `7c6bfb1` Add git-subtree-check.sh script
- bitcoin/bitcoin#6033 `1623f6e` FreeBSD, OpenBSD thread renaming
- bitcoin/bitcoin#6064 `b46e7c2` Several changes to mruset
- bitcoin/bitcoin#6104 `3e2559c` Show an init message while activating best chain
- bitcoin/bitcoin#6125 `351f73e` Clean up parsing of bool command line args
- bitcoin/bitcoin#5964 `b4c219b` Lightweight task scheduler
- bitcoin/bitcoin#6116 `30dc3c1` [OSX] rename Bitcoin-Qt.app to Bitcoin-Core.app
- bitcoin/bitcoin#6168 `b3024f0` contrib/linearize: Support linearization of testnet blocks
- bitcoin/bitcoin#6098 `7708fcd` Update Windows resource files (and add one for bitcoin-tx)
- bitcoin/bitcoin#6159 `e1412d3` Catch errors on datadir lock and pidfile delete
- bitcoin/bitcoin#6186 `182686c` Fix two problems in CSubnet parsing
- bitcoin/bitcoin#6174 `df992b9` doc: add translation strings policy
- bitcoin/bitcoin#6210 `dfdb6dd` build: disable optional use of gmp in internal secp256k1 build
- bitcoin/bitcoin#6264 `94cd705` Remove translation for -help-debug options
- bitcoin/bitcoin#6286 `3902c15` Remove berkeley-db4 workaround in MacOSX build docs
- bitcoin/bitcoin#6319 `3f8fcc9` doc: update mailing list address
- bitcoin/bitcoin#6438 `2531438` openssl: avoid config file load/race
- bitcoin/bitcoin#6439 `980f820` Updated URL location of netinstall for Debian
- bitcoin/bitcoin#6384 `8e5a969` qt: Force TLS1.0+ for SSL connections
- bitcoin/bitcoin#6471 `92401c2` Depends: bump to qt 5.5
- bitcoin/bitcoin#6224 `93b606a` Be even stricter in processing unrequested blocks
- bitcoin/bitcoin#6571 `100ac4e` libbitcoinconsensus: avoid a crash in multi-threaded environments
- bitcoin/bitcoin#6545 `649f5d9` Do not store more than 200 timedata samples.
- bitcoin/bitcoin#6694 `834e299` [QT] fix thin space word wrap line break issue
- bitcoin/bitcoin#6703 `1cd7952` Backport bugfixes to 0.11
- bitcoin/bitcoin#6750 `5ed8d0b` Recent rejects backport to v0.11
- bitcoin/bitcoin#6769 `71cc9d9` Test LowS in standardness, removes nuisance malleability vector.
- bitcoin/bitcoin#6789 `b4ad73f` Update miniupnpc to 1.9.20151008
- bitcoin/bitcoin#6785 `b4dc33e` Backport to v0.11: In (strCommand == "tx"), return if AlreadyHave()
- bitcoin/bitcoin#6412 `0095b9a` Test whether created sockets are select()able
- bitcoin/bitcoin#6795 `4dbcec0` net: Disable upnp by default
- bitcoin/bitcoin#6793 `e7bcc4a` Bump minrelaytxfee default
- bitcoin/bitcoin#6124 `684636b` Make CScriptNum() take nMaxNumSize as an argument
- bitcoin/bitcoin#6124 `4fa7a04` Replace NOP2 with CHECKLOCKTIMEVERIFY (BIP65)
- bitcoin/bitcoin#6124 `6ea5ca4` Enable CHECKLOCKTIMEVERIFY as a standard script verify flag
- bitcoin/bitcoin#6351 `5e82e1c` Add CHECKLOCKTIMEVERIFY (BIP65) soft-fork logic
- bitcoin/bitcoin#6353 `ba1da90` Show softfork status in getblockchaininfo
- bitcoin/bitcoin#6351 `6af25b0` Add BIP65 to getblockchaininfo softforks list
- bitcoin/bitcoin#6688 `01878c9` Fix locking in GetTransaction
- bitcoin/bitcoin#6653 `b3eaa30` [Qt] Raise debug window when requested
- bitcoin/bitcoin#6600 `1e672ae` Debian/Ubuntu: Include bitcoin-tx binary
- bitcoin/bitcoin#6600 `2394f4d` Debian/Ubuntu: Split bitcoin-tx into its own package
- bitcoin/bitcoin#5987 `33d6825` Bugfix: Allow mining on top of old tip blocks for testnet
- bitcoin/bitcoin#6852 `21e58b8` build: make sure OpenSSL heeds noexecstack
- bitcoin/bitcoin#6846 `af6edac` alias `-h` for `--help`
- bitcoin/bitcoin#6867 `95a5039` Set TCP_NODELAY on P2P sockets.
- bitcoin/bitcoin#6856 `dfe55bd` Do not allow blockfile pruning during reindex.
- bitcoin/bitcoin#6566 `a1d3c6f` Add rules--presently disabled--for using GetMedianTimePast as end point for lock-time calculations
- bitcoin/bitcoin#6566 `f720c5f` Enable policy enforcing GetMedianTimePast as the end point of lock-time constraints
- bitcoin/bitcoin#6917 `0af5b8e` leveldb: Win32WritableFile without memory mapping
- bitcoin/bitcoin#6948 `4e895b0` Always flush block and undo when switching to new file

Credits
-------

Thanks to everyone who directly contributed to this release:

- 21E14
- Adam Weiss
- Alex Morcos
- ayeowch
- azeteki
- Ben Holden-Crowther
- bikinibabe
- BitcoinPRReadingGroup
- Blake Jakopovic
- BtcDrak
- Casey Rodarmor
- charlescharles
- Chris Arnesen
- Chris Kleeschulte
- Ciemon
- CohibAA
- Corinne Dashjr
- Cory Fields
- Cozz Lovan
- Daira Hopwood
- Daniel Cousens
- Daniel Kraft
- Dave Collins
- David A. Harding
- dexX7
- Diego Viola
- Earlz
- Eric Lombrozo
- Eric R. Schulz
- Esteban Ordano
- Everett Forth
- fanquake
- Flavien Charlon
- fsb4000
- Gavin Andresen
- Gregory Maxwell
- Heath
- Ivan Pustogarov
- Jacob Welsh
- Jameson Lopp
- Jason Lewicki
- Jeff Garzik
- Jonas Schnelli
- Jonathan Brown
- Jorge Timón
- joshr
- J Ross Nicoll
- jtimon
- Julian Yap
- Luca Venturini
- Luke Dashjr
- Manuel Araoz
- MarcoFalke
- Marco Falke
- Mark Friedenbach
- Matt Bogosian
- Matt Corallo
- Micha
- Michael Ford
- Mike Hearn
- Mitchell Cash
- mrbandrews
- Nicolas Benoit
- paveljanik
- Pavel Janík
- Pavel Vasin
- Peter Todd
- Philip Kaufmann
- Pieter Wuille
- pstratem
- randy-waterhouse
- rion
- Rob Van Mieghem
- Ross Nicoll
- Ruben de Vries
- sandakersmann
- Shaul Kfir
- Shawn Wilkinson
- sinetek
- Suhas Daftuar
- svost
- tailsjoin
- ฿tcDrak
- Thomas Zander
- Tom Harding
- UdjinM6
- Veres Lajos
- Vitalii Demianets
- Wladimir J. van der Laan
- Zak Wilcox

And all those who contributed additional code review and/or security research:

- Sergio Demian Lerner
- timothy on IRC for reporting the issue
- Vulnerability in miniupnp discovered by Aleksandar Nikolic of Cisco Talos

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
