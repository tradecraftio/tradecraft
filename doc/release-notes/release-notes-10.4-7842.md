Freicoin version 10.4 is now available from:

  * [Linux 32-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4-7842-linux32.zip)
  * [Linux 64-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4-7842-linux64.zip)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4-7842-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4-7842-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4-7842-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4-7842-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4-7842-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4-7842-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v10.4-7842.zip)

This is a new major version release, bringing both new features and
bug fixes.

Please report bugs using the issue tracker at github:

  https://github.com/tradecraftio/tradecraft/issues

Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Freicoin-Qt (on Mac) or
freicoind/freicoin-qt (on Linux).

Downgrading warning
---------------------

Because release 10.4 makes use of headers-first synchronization and parallel
block download (see further), the block files and databases are not
backwards-compatible with older versions of Freicoin or other software:

* Blocks will be stored on disk out of order (in the order they are
received, really), which makes it incompatible with some tools or
other programs. Reindexing using earlier versions will also not work
anymore as a result of this.

* The block index database will now hold headers for which no block is
stored on disk, which earlier versions won't support.

If you want to be able to downgrade smoothly, make a backup of your entire data
directory. Without this your node will need start syncing (or importing from
bootstrap.dat) anew afterwards. It is possible that the data from a completely
synchronized 0.10 node may be usable in older versions as-is, but this is not
supported and may break as soon as the older version attempts to reindex.

This does not affect wallet forward or backward compatibility.


Notable changes
===============

This fixes a serious problem on Windows with data directories that have non-ASCII
characters (https://github.com/bitcoin/bitcoin/issues/6078).

Protocol-cleanup flag day fork
------------------------------

To achieve desired scaling limits, the forward blocks protocol upgrade
will eventually trigger a hard-fork modification of the consensus
rules, for the primary purposes of dropping enforcement of many
aggregate block limits and altering the difficulty adjustment
algorithm.

This hard-fork will not activate until it is absolutely necessary for
it to do so, at the point when measurements of real demand for
additional shard space in aggregate across all forward block
shard-chains exceeds the available space in the compatibility
chain. It is anticipated that this will not occur until many, many
years into the future, when Freicoin/Tradecraft's usage exceeds even
the levels of bitcoin usage ca. 2018. However when it does eventually
trigger, any node enforcing the old rules will be left behind.

Beginning in 10.4, we introduce a flag-day relaxation of the consensus
rules in preparation for this eventual fork. Since the rule changes
for forward blocks have not been written yet, any code written now
wouldn't be able to detect actual activation or enforce the new
aggregate limits. Instead we schedule a relaxation of the consensus
rules at the EOL support date for the current release, after which
rules which we anticipate being changed are simply unenforced, and
aggregate limits are set to the maximum values the software is able to
support. After the flag-day, older clients of at least version 10.4
will continue to receive blocks, but with only SPV security ("trust
the most work") for the new protocol rules. So activation of forward
blocks' new scaling limits becomes a soft-fork starting with the
release of 10.4, with the only concern being the forking off of
pre-10.4 nodes upon activation.

The protocol cleanup rule change is scheduled for activation on 2
April 2021 at midnight UTC. This is 4PM PDT, 7PM EDT, and 9AM
JST. Since the activation time is median-time-past, it'll actually
trigger about an hour after this wall-clock time.

This date is chosen to be roughly 2 years after the expected release
date of official binaries for 10.4. While the Freicoin developer team
doesn't have the resources to provide strong ongoing support beyond
emergency fixes, we nevertheless have an ideal goal of supporting
release binaries for up to 2 years following the first release from
that series. Any release of a new series prior to the deployment of
forward blocks will reset this to be at least two years from the time
of release. When forward blocks is deployed, this parameter will be
set to the highest value used in any prior release, and becomes the
earliest time at which the hard-fork rules can activate.

All users should be aware that timely updates or modification of their
own nodes are required within this time window in order to maintain
full-node security, until such time as a version supporting forward
blocks is released and adopted. Miners especially *must* upgrade or
modify their block-generating nodes before this date, or else they
place the consensus of the network at risk.

Depreciation of 32-bit clients
------------------------------

The lifting of aggregate limits in preparation of the new scaling
limits enabled by forward blocks unfortunately opens a memory
exhaustion denial of service attack vector after the above-mentioned
flag-day activation of new rules, even if the actual activation date
has been pushed back in later releases. To protect against this,
32-bit clients have smaller network buffers (about 16 MiB under
default settings), too small to contain the largest block-relay
message allowed under the new rules (2 GiB).

This unfortunately means that if/when forward blocks is deployed and
the flexible cap used to grow aggregate block size limits, then 32-bit
nodes will no longer be able to perform network synchronization once a
block larger than 17,179,845 bytes is included into the main chain.

For this reason, support for 32-bit clients is officially depreciated
as of the 10.4 release. Starting with 10.4, 32-bit clients will at
some point in the future be unable to sync the main chain. Users on
32-bit hosts should strongly consider upgrading before activation of
the new rules.

However should you need to continue running 32-bit nodes, be advised
that the network message limits are informed by the -maxconnections
option. Specifically the maximum network packet size allowed is

    2^32 / max(125, -maxconnections) / 2,

which for the default of 125 connection slots is 17,179,869 bytes (not
including the 24-byte header). By providing a lower value for
-maxconnections this value is increased.  With -maxconnections=1, the
calculated value is clamped to MAX_BLOCKFILE_SIZE. So a 32-bit node
with -maxconnections=1 will be able to network synchronize even the
largest blocks from its (only) peer.

Although depreciated, 32-bit official binaries will continue to be
provided for releases so long as it remains a reasonable amount of
work to do so.

Faster synchronization
----------------------

Freicoin now uses 'headers-first synchronization'. This means that we first
ask peers for block headers (a total of 19 megabytes, as of February 2019) and
validate those. In a second stage, when the headers have been discovered, we
download the blocks. However, as we already know about the whole chain in
advance, the blocks can be downloaded in parallel from all available peers.

In practice, this means a much faster and more robust synchronization. You may
notice a slower progress in the very first few minutes, when headers are still
being fetched and verified, but it should gain speed afterwards.

A few RPCs were added/updated as a result of this:
- `getblockchaininfo` now returns the number of validated headers in addition to
the number of validated blocks.
- `getpeerinfo` lists both the number of blocks and headers we know we have in
common with each peer. While synchronizing, the heights of the blocks that we
have requested from peers (but haven't received yet) are also listed as
'inflight'.
- A new RPC `getchaintips` lists all known branches of the block chain,
including those we only have headers for.

Transaction fee changes
-----------------------

This release automatically estimates how high a transaction fee (or how
high a priority) transactions require to be confirmed quickly. The default
settings will create transactions that confirm quickly; see the new
'txconfirmtarget' setting to control the tradeoff between fees and
confirmation times. Fees are added by default unless the 'sendfreetransactions'
setting is enabled.

Prior releases used hard-coded fees (and priorities), and would
sometimes create transactions that took a very long time to confirm.

Statistics used to estimate fees and priorities are saved in the
data directory in the `fee_estimates.dat` file just before
program shutdown, and are read in at startup.

New command line options for transaction fee changes:
- `-txconfirmtarget=n` : create transactions that have enough fees (or priority)
so they are likely to begin confirmation within n blocks (default: 1). This setting
is over-ridden by the -paytxfee option.
- `-sendfreetransactions` : Send transactions as zero-fee transactions if possible
(default: 0)

New RPC commands for fee estimation:
- `estimatefee nblocks` : Returns approximate fee-per-1,000-bytes needed for
a transaction to begin confirmation within nblocks. Returns -1 if not enough
transactions have been observed to compute a good estimate.
- `estimatepriority nblocks` : Returns approximate priority needed for
a zero-fee transaction to begin confirmation within nblocks. Returns -1 if not
enough free transactions have been observed to compute a good
estimate.

RPC access control changes
--------------------------

Subnet matching for the purpose of access control is now done
by matching the binary network address, instead of with string wildcard matching.
For the user this means that `-rpcallowip` takes a subnet specification, which can be

- a single IP address (e.g. `1.2.3.4` or `fe80::0012:3456:789a:bcde`)
- a network/CIDR (e.g. `1.2.3.0/24` or `fe80::0000/64`)
- a network/netmask (e.g. `1.2.3.4/255.255.255.0` or `fe80::0012:3456:789a:bcde/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff`)

An arbitrary number of `-rpcallow` arguments can be given. An incoming connection will be accepted if its origin address
matches one of them.

For example:

| 0.9.x and before                           | 10.x                                  |
|--------------------------------------------|---------------------------------------|
| `-rpcallowip=192.168.1.1`                  | `-rpcallowip=192.168.1.1` (unchanged) |
| `-rpcallowip=192.168.1.*`                  | `-rpcallowip=192.168.1.0/24`          |
| `-rpcallowip=192.168.*`                    | `-rpcallowip=192.168.0.0/16`          |
| `-rpcallowip=*` (dangerous!)               | `-rpcallowip=::/0` (still dangerous!) |

Using wildcards will result in the rule being rejected with the following error in debug.log:

    Error: Invalid -rpcallowip subnet specification: *. Valid are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0) or a network/CIDR (e.g. 1.2.3.4/24).


REST interface
--------------

A new HTTP API is exposed when running with the `-rest` flag, which allows
unauthenticated access to public node data.

It is served on the same port as RPC, but does not need a password, and uses
plain HTTP instead of JSON-RPC.

Assuming a local RPC server running on port 8332, it is possible to request:
- Blocks: http://localhost:8332/rest/block/*HASH*.*EXT*
- Blocks without transactions: http://localhost:8332/rest/block/notxdetails/*HASH*.*EXT*
- Transactions (requires `-txindex`): http://localhost:8332/rest/tx/*HASH*.*EXT*

In every case, *EXT* can be `bin` (for raw binary data), `hex` (for hex-encoded
binary) or `json`.

For more details, see the `doc/REST-interface.md` document in the repository.

RPC Server "Warm-Up" Mode
-------------------------

The RPC server is started earlier now, before most of the expensive
initializations like loading the block index.  It is available now almost
immediately after starting the process.  However, until all initializations
are done, it always returns an immediate error with code -28 to all calls.

This new behavior can be useful for clients to know that a server is already
started and will be available soon (for instance, so that they do not
have to start it themselves).

Improved signing security
-------------------------

For 0.10 the security of signing against unusual attacks has been
improved by making the signatures constant time and deterministic.

This change is a result of switching signing to use libsecp256k1
instead of OpenSSL. Libsecp256k1 is a cryptographic library
optimized for the curve Freicoin uses which was created by Bitcoin
Core developer Pieter Wuille.

There exist attacks[1] against most ECC implementations where an
attacker on shared virtual machine hardware could extract a private
key if they could cause a target to sign using the same key hundreds
of times. While using shared hosts and reusing keys are inadvisable
for other reasons, it's a better practice to avoid the exposure.

OpenSSL has code in their source repository for derandomization
and reduction in timing leaks that we've eagerly wanted to use for a
long time, but this functionality has still not made its
way into a released version of OpenSSL. Libsecp256k1 achieves
significantly stronger protection: As far as we're aware this is
the only deployed implementation of constant time signing for
the curve Freicoin uses and we have reason to believe that
libsecp256k1 is better tested and more thoroughly reviewed
than the implementation in OpenSSL.

[1] https://eprint.iacr.org/2014/161.pdf

Watch-only wallet support
-------------------------

The wallet can now track transactions to and from wallets for which you know
all addresses (or scripts), even without the private keys.

This can be used to track payments without needing the private keys online on a
possibly vulnerable system. In addition, it can help for (manual) construction
of multisig transactions where you are only one of the signers.

One new RPC, `importaddress`, is added which functions similarly to
`importprivkey`, but instead takes an address or script (in hexadecimal) as
argument.  After using it, outputs credited to this address or script are
considered to be received, and transactions consuming these outputs will be
considered to be sent.

The following RPCs have optional support for watch-only:
`getbalance`, `listreceivedbyaddress`, `listreceivedbyaccount`,
`listtransactions`, `listaccounts`, `listsinceblock`, `gettransaction`. See the
RPC documentation for those methods for more information.

Compared to using `getrawtransaction`, this mechanism does not require
`-txindex`, scales better, integrates better with the wallet, and is compatible
with future block chain pruning functionality. It does mean that all relevant
addresses need to added to the wallet before the payment, though.

Consensus library
-----------------

Starting from 10.4, the Freicoin distribution includes a consensus library.

The purpose of this library is to make the verification functionality that is
critical to Freicoin's consensus available to other applications, e.g. to language
bindings such as [python-bitcoinlib](https://pypi.python.org/pypi/python-bitcoinlib) or
alternative node implementations.

This library is called `libfreicoinconsensus.so` (or, `.dll` for Windows).
Its interface is defined in the C header [freicoinconsensus.h](https://github.com/tradecraftio/tradecraft/blob/v10.4/src/script/freicoinconsensus.h).

In its initial version the API includes two functions:

- `freicoinconsensus_verify_script` verifies a script. It returns whether the indicated input of the provided serialized transaction
correctly spends the passed scriptPubKey under additional constraints indicated by flags
- `freicoinconsensus_version` returns the API version, currently at an experimental `0`

The functionality is planned to be extended to e.g. UTXO management in upcoming releases, but the interface
for existing methods should remain stable.

Standard script rules relaxed for P2SH addresses
------------------------------------------------

The IsStandard() rules have been almost completely removed for P2SH
redemption scripts, allowing applications to make use of any valid
script type, such as "n-of-m OR y", hash-locked oracle addresses, etc.
While the Freicoin protocol has always supported these types of script,
actually using them on mainnet has been previously inconvenient as
standard Freicoin nodes wouldn't relay them to miners, nor would
most miners include them in blocks they mined.

freicoin-tx
-----------

It has been observed that many of the RPC functions offered by freicoind are
"pure functions", and operate independently of the freicoind wallet. This
included many of the RPC "raw transaction" API functions, such as
createrawtransaction.

freicoin-tx is a newly introduced command line utility designed to enable easy
manipulation of freicoin transactions. A summary of its operation may be
obtained via "freicoin-tx --help" Transactions may be created or signed in a
manner similar to the RPC raw tx API. Transactions may be updated, deleting
inputs or outputs, or appending new inputs and outputs. Custom scripts may be
easily composed using a simple text notation, borrowed from the freicoin test
suite.

This tool may be used for experimenting with new transaction types, signing
multi-party transactions, and many other uses. Long term, the goal is to
deprecate and remove "pure function" RPC API calls, as those do not require a
server round-trip to execute.

Other utilities "freicoin-key" and "freicoin-script" have been proposed, making
key and script operations easily accessible via command line.

Mining and relay policy enhancements
------------------------------------

Freicoin's block templates are now for version 3 blocks only, and any mining
software relying on its `getblocktemplate` must be updated in parallel to use
libblkmaker either version 0.4.2 or any version from 0.5.1 onward.
If you are solo mining, this will affect you the moment you upgrade Freicoin.
If you are mining with the stratum mining protocol: this does not affect you.
If you are mining with the getblocktemplate protocol to a pool: this will affect
you at the pool operator's discretion.

The `prioritisetransaction` RPC method has been added to enable miners to
manipulate the priority of transactions on an individual basis.

Freicoin now supports BIP 22 long polling, so mining software can be
notified immediately of new templates rather than having to poll periodically.

Support for BIP 23 block proposals is now available in Freicoin's
`getblocktemplate` method. This enables miners to check the basic validity of
their next block before expending work on it, reducing risks of accidental
hardforks or mining invalid blocks.

The relay policy has changed to more properly implement the desired behavior of not
relaying free (or very low fee) transactions unless they have a priority above the
AllowFreeThreshold(), in which case they are relayed subject to the rate limiter.

Fix buffer overflow in bundled upnp
------------------------------------

Bundled miniupnpc was updated to 1.9.20151008. This fixes a buffer overflow in
the XML parser during initial network discovery.

Details can be found here: http://talosintel.com/reports/TALOS-2015-0035/

This applies to the distributed executables only, not when building from source or
using distribution provided packages.

Additionally, upnp has been disabled by default. This may result in a lower
number of reachable nodes on IPv4, however this prevents future libupnpc
vulnerabilities from being a structural risk to the network
(see https://github.com/bitcoin/bitcoin/pull/6795).

Minimum relay fee default increase
-----------------------------------

The default for the `-minrelaytxfee` setting has been increased from `0.00001`
to `0.00005`.

This is antemporary measure, bridging the time until a dynamic method for
determining this fee is merged (which will be in 0.12).

(see https://github.com/bitcoin/bitcoin/pull/6793, as well as the 11.3
release notes, in which this value was suggested)

Windows bug fix for corrupted UTXO database on unclean shutdowns
----------------------------------------------------------------

Several Windows users of the upstream Bitcoin Core software reported
that they often need to reindex the entire blockchain after an unclean
shutdown of Bitcoin on Windows (or an unclean shutdown of Windows
itself). Although unclean shutdowns remain unsafe, this release no
longer relies on memory-mapped files for the UTXO database, which
significantly reduced the frequency of unclean shutdowns leading to
required reindexes during testing.

For more information, see: <https://github.com/bitcoin/bitcoin/pull/6917>

Other fixes for database corruption on Windows are expected in the
next major release.

10.4 Change log
===============

Detailed release notes follow. This overview includes changes that affect external
behavior, not code moves, refactors or string updates.

RPC:
- `f923c07` Support IPv6 lookup in bitcoin-cli even when IPv6 only bound on localhost
- `b641c9c` Fix addnode "onetry": Connect with OpenNetworkConnection
- `171ca77` estimatefee / estimatepriority RPC methods
- `b750cf1` Remove cli functionality from bitcoind
- `f6984e8` Add "chain" to getmininginfo, improve help in getblockchaininfo
- `99ddc6c` Add nLocalServices info to RPC getinfo
- `cf0c47b` Remove getwork() RPC call
- `2a72d45` prioritisetransaction <txid> <priority delta> <priority tx fee>
- `2ec5a3d` Prevent easy RPC memory exhaustion attack
- `d4640d7` Added argument to getbalance to include watchonly addresses and fixed errors in balance calculation
- `83f3543` Added argument to listaccounts to include watchonly addresses
- `952877e` Showing 'involvesWatchonly' property for transactions returned by 'listtransactions' and 'listsinceblock'. It is only appended when the transaction involves a watchonly address
- `d7d5d23` Added argument to listtransactions and listsinceblock to include watchonly addresses
- `f87ba3d` added includeWatchonly argument to 'gettransaction' because it affects balance calculation
- `0fa2f88` added includedWatchonly argument to listreceivedbyaddress/...account
- `6c37f7f` `getrawchangeaddress`: fail when keypool exhausted and wallet locked
- `ff6a7af` getblocktemplate: longpolling support
- `c4a321f` Add peerid to getpeerinfo to allow correlation with the logs
- `1b4568c` Add vout to ListTransactions output
- `b33bd7a` Implement "getchaintips" RPC command to monitor blockchain forks
- `733177e` Remove size limit in RPC client, keep it in server
- `6b5b7cb` Categorize rpc help overview
- `6f2c26a` Closely track mempool byte total. Add "getmempoolinfo" RPC
- `aa82795` Add detailed network info to getnetworkinfo RPC
- `01094bd` Don't reveal whether password is <20 or >20 characters in RPC
- `57153d4` rpc: Compute number of confirmations of a block from block height
- `ff36cbe` getnetworkinfo: export local node's client sub-version string
- `d14d7de` SanitizeString: allow '(' and ')'
- `31d6390` Fixed setaccount accepting foreign address
- `b5ec5fe` update getnetworkinfo help with subversion
- `ad6e601` RPC additions after headers-first
- `33dfbf5` rpc: Fix leveldb iterator leak, and flush before `gettxoutsetinfo`
- `f877aaa` submitblock: Use a temporary CValidationState to determine accurately the outcome of ProcessBlock
- `e69a587` submitblock: Support for returning specific rejection reasons
- `af82884` Add "warmup mode" for RPC server
- `e2655e0` Add unauthenticated HTTP REST interface to public blockchain data
- `683dc40` Disable SSLv3 (in favor of TLS) for the RPC client and server
- `44b4c0d` signrawtransaction: validate private key
- `9765a50` Implement BIP 23 Block Proposal
- `f9de17e` Add warning comment to getinfo
- `7f502be` fix crash: createmultisig and addmultisigaddress
- `eae305f` Fix missing lock in submitblock

Command-line options:
- `ee21912` Use netmasks instead of wildcards for IP address matching
- `deb3572` Add `-rpcbind` option to allow binding RPC port on a specific interface
- `96b733e` Add `-version` option to get just the version
- `1569353` Add `-stopafterblockimport` option
- `77cbd46` Let -zapwallettxes recover transaction meta data
- `1c750db` remove -tor compatibility code (only allow -onion)
- `4aaa017` rework help messages for fee-related options
- `4278b1d` Clarify error message when invalid -rpcallowip
- `6b407e4` -datadir is now allowed in config files
- `bdd5b58` Add option `-sysperms` to disable 077 umask (create new files with system default umask)
- `cbe39a3` Add "bitcoin-tx" command line utility and supporting modules
- `dbca89b` Trigger -alertnotify if network is upgrading without you
- `ad96e7c` Make -reindex cope with out-of-order blocks
- `16d5194` Skip reindexed blocks individually
- `ec01243` --tracerpc option for regression tests
- `f654f00` Change -genproclimit default to 1
- `3c77714` Make -proxy set all network types, avoiding a connect leak
- `57be955` Remove -printblock, -printblocktree, and -printblockindex
- `ad3d208` remove -maxorphanblocks config parameter since it is no longer functional

Block and transaction handling:
- `7a0e84d` ProcessGetData(): abort if a block file is missing from disk
- `8c93bf4` LoadBlockIndexDB(): Require block db reindex if any `blk*.dat` files are missing
- `77339e5` Get rid of the static chainMostWork (optimization)
- `4e0eed8` Allow ActivateBestChain to release its lock on cs_main
- `18e7216` Push cs_mains down in ProcessBlock
- `fa126ef` Avoid undefined behavior using CFlatData in CScript serialization
- `7f3b4e9` Relax IsStandard rules for pay-to-script-hash transactions
- `c9a0918` Add a skiplist to the CBlockIndex structure
- `bc42503` Use unordered_map for CCoinsViewCache with salted hash (optimization)
- `d4d3fbd` Do not flush the cache after every block outside of IBD (optimization)
- `ad08d0b` Bugfix: make CCoinsViewMemPool support pruned entries in underlying cache
- `5734d4d` Only remove actually failed blocks from setBlockIndexValid
- `d70bc52` Rework block processing benchmark code
- `714a3e6` Only keep setBlockIndexValid entries that are possible improvements
- `ea100c7` Reduce maximum coinscache size during verification (reduce memory usage)
- `4fad8e6` Reject transactions with excessive numbers of sigops
- `b0875eb` Allow BatchWrite to destroy its input, reducing copying (optimization)
- `92bb6f2` Bypass reloading blocks from disk (optimization)
- `2e28031` Perform CVerifyDB on pcoinsdbview instead of pcoinsTip (reduce memory usage)
- `ab15b2e` Avoid copying undo data (optimization)
- `341735e` Headers-first synchronization
- `afc32c5` Fix rebuild-chainstate feature and improve its performance
- `e11b2ce` Fix large reorgs
- `ed6d1a2` Keep information about all block files in memory
- `a48f2d6` Abstract context-dependent block checking from acceptance
- `7e615f5` Fixed mempool sync after sending a transaction
- `51ce901` Improve chainstate/blockindex disk writing policy
- `a206950` Introduce separate flushing modes
- `9ec75c5` Add a locking mechanism to IsInitialBlockDownload to ensure it never goes from false to true
- `868d041` Remove coinbase-dependant transactions during reorg
- `723d12c` Remove txn which are invalidated by coinbase maturity during reorg
- `0cb8763` Check against MANDATORY flags prior to accepting to mempool
- `8446262` Reject headers that build on an invalid parent
- `008138c` Bugfix: only track UTXO modification after lookup
- `1d2cdd2` Fix InvalidateBlock to add chainActive.Tip to setBlockIndexCandidates
- `c91c660` fix InvalidateBlock to repopulate setBlockIndexCandidates
- `002c8a2` fix possible block db breakage during re-index
- `a1f425b` Add (optional) consistency check for the block chain data structures
- `1c62e84` Keep mempool consistent during block-reorgs
- `57d1f46` Fix CheckBlockIndex for reindex
- `bac6fca` Set nSequenceId when a block is fully linked

P2P protocol and network code:
- `f80cffa` Do not trigger a DoS ban if SCRIPT_VERIFY_NULLDUMMY fails
- `c30329a` Add testnet DNS seed of Alex Kotenko
- `45a4baf` Add testnet DNS seed of Andreas Schildbach
- `f1920e8` Ping automatically every 2 minutes (unconditionally)
- `806fd19` Allocate receive buffers in on the fly
- `6ecf3ed` Display unknown commands received
- `aa81564` Track peers' available blocks
- `caf6150` Use async name resolving to improve net thread responsiveness
- `9f4da19` Use pong receive time rather than processing time
- `0127a9b` remove SOCKS4 support from core and GUI, use SOCKS5
- `40f5cb8` Send rejects and apply DoS scoring for errors in direct block validation
- `dc942e6` Introduce whitelisted peers
- `c994d2e` prevent SOCKET leak in BindListenPort()
- `a60120e` Add built-in seeds for .onion
- `60dc8e4` Allow -onlynet=onion to be used
- `3a56de7` addrman: Do not propagate obviously poor addresses onto the network
- `6050ab6` netbase: Make SOCKS5 negotiation interruptible
- `604ee2a` Remove tx from AlreadyAskedFor list once we receive it, not when we process it
- `efad808` Avoid reject message feedback loops
- `71697f9` Separate protocol versioning from clientversion
- `20a5f61` Don't relay alerts to peers before version negotiation
- `b4ee0bd` Introduce preferred download peers
- `845c86d` Do not use third party services for IP detection
- `12a49ca` Limit the number of new addresses to accumulate
- `35e408f` Regard connection failures as attempt for addrman
- `a3a7317` Introduce 10 minute block download timeout
- `3022e7d` Require sufficent priority for relay of free transactions
- `58fda4d` Update seed IPs, based on bitcoin.sipa.be crawler data
- `18021d0` Remove bitnodes.io from dnsseeds.
- `78f64ef` don't trickle for whitelisted nodes
- `ca301bf` Reduce fingerprinting through timestamps in 'addr' messages.
- `200f293` Ignore getaddr messages on Outbound connections.
- `d5d8998` Limit message sizes before transfer
- `aeb9279` Better fingerprinting protection for non-main-chain getdatas.
- `cf0218f` Make addrman's bucket placement deterministic (countermeasure 1 against eclipse attacks, see http://cs-people.bu.edu/heilman/eclipse/)
- `0c6f334` Always use a 50% chance to choose between tried and new entries (countermeasure 2 against eclipse attacks)
- `214154e` Do not bias outgoing connections towards fresh addresses (countermeasure 2 against eclipse attacks)
- `aa587d4` Scale up addrman (countermeasure 6 against eclipse attacks)
- `139cd81` Cap nAttempts penalty at 8 and switch to pow instead of a division loop

Validation:
- `6fd7ef2` Also switch the (unused) verification code to low-s instead of even-s
- `584a358` Do merkle root and txid duplicates check simultaneously
- `217a5c9` When transaction outputs exceed inputs, show the offending amounts so as to aid debugging
- `f74fc9b` Print input index when signature validation fails, to aid debugging
- `6fd59ee` script.h: set_vch() should shift a >32 bit value
- `d752ba8` Add SCRIPT_VERIFY_SIGPUSHONLY (BIP62 rule 2) (test only)
- `698c6ab` Add SCRIPT_VERIFY_MINIMALDATA (BIP62 rules 3 and 4) (test only)
- `ab9edbd` script: create sane error return codes for script validation and remove logging
- `219a147` script: check ScriptError values in script tests
- `0391423` Discourage NOPs reserved for soft-fork upgrades
- `98b135f` Make STRICTENC invalid pubkeys fail the script rather than the opcode
- `307f7d4` Report script evaluation failures in log and reject messages
- `ace39db` consensus: guard against openssl's new strict DER checks
- `12b7c44` Improve robustness of DER recoding code
- `76ce5c8` fail immediately on an empty signature
- `d148f62` Acquire CCheckQueue's lock to avoid race condition

Build system:
- `f25e3ad` Fix build in OS X 10.9
- `65e8ba4` build: Switch to non-recursive make
- `460b32d` build: fix broken boost chrono check on some platforms
- `9ce0774` build: Fix windows configure when using --with-qt-libdir
- `ea96475` build: Add mention of --disable-wallet to bdb48 error messages
- `1dec09b` depends: add shared dependency builder
- `c101c76` build: Add --with-utils (bitcoin-cli and bitcoin-tx, default=yes). Help string consistency tweaks. Target sanity check fix
- `e432a5f` build: add option for reducing exports (v2)
- `6134b43` Fixing condition 'sabotaging' MSVC build
- `af0bd5e` osx: fix signing to make Gatekeeper happy (again)
- `a7d1f03` build: fix dynamic boost check when --with-boost= is used
- `d5fd094` build: fix qt test build when libprotobuf is in a non-standard path
- `2cf5f16` Add libbitcoinconsensus library
- `914868a` build: add a deterministic dmg signer 
- `2d375fe` depends: bump openssl to 1.0.1k
- `b7a4ecc` Build: Only check for boost when building code that requires it
- `8752b5c` 0.10 fix for crashes on OSX 10.6

Wallet:
- `b33d1f5` Use fee/priority estimates in wallet CreateTransaction
- `4b7b1bb` Sanity checks for estimates
- `c898846` Add support for watch-only addresses
- `d5087d1` Use script matching rather than destination matching for watch-only
- `d88af56` Fee fixes
- `a35b55b` Don't run full check every time we decrypt wallet
- `3a7c348` Fix make_change to not create half-satoshis
- `f606bb9` fix a possible memory leak in CWalletDB::Recover
- `870da77` fix possible memory leaks in CWallet::EncryptWallet
- `ccca27a` Watch-only fixes
- `9b1627d` [Wallet] Reduce minTxFee for transaction creation to 1000 satoshis
- `a53fd41` Deterministic signing
- `15ad0b5` Apply AreSane() checks to the fees from the network
- `11855c1` Enforce minRelayTxFee on wallet created tx and add a maxtxfee option
- `824c011` fix boost::get usage with boost 1.58

GUI:
- `c21c74b` osx: Fix missing dock menu with qt5
- `b90711c` Fix Transaction details shows wrong To:
- `516053c` Make links in 'About Bitcoin Core' clickable
- `bdc83e8` Ensure payment request network matches client network
- `65f78a1` Add GUI view of peer information
- `06a91d9` VerifyDB progress reporting
- `fe6bff2` Add BerkeleyDB version info to RPCConsole
- `b917555` PeerTableModel: Fix potential deadlock. #4296
- `dff0e3b` Improve rpc console history behavior
- `95a9383` Remove CENT-fee-rule from coin control completely
- `56b07d2` Allow setting listen via GUI
- `d95ba75` Log messages with type>QtDebugMsg as non-debug
- `8969828` New status bar Unit Display Control and related changes
- `674c070` seed OpenSSL PNRG with Windows event data
- `509f926` Payment request parsing on startup now only changes network if a valid network name is specified
- `acd432b` Prevent balloon-spam after rescan
- `7007402` Implement SI-style (thin space) thoudands separator
- `91cce17` Use fixed-point arithmetic in amount spinbox
- `bdba2dd` Remove an obscure option no-one cares about
- `bd0aa10` Replace the temporary file hack currently used to change Bitcoin-Qt's dock icon (OS X) with a buffer-based solution
- `94e1b9e` Re-work overviewpage UI
- `8bfdc9a` Better looking trayicon
- `b197bf3` disable tray interactions when client model set to 0
- `1c5f0af` Add column Watch-only to transactions list
- `21f139b` Fix tablet crash. closes #4854
- `e84843c` Broken addresses on command line no longer trigger testnet
- `a49f11d` Change splash screen to normal window
- `1f9be98` Disable App Nap on OSX 10.9+
- `27c3e91` Add proxy to options overridden if necessary
- `4bd1185` Allow "emergency" shutdown during startup
- `d52f072` Don't show wallet options in the preferences menu when running with -disablewallet
- `6093aa1` Qt: QProgressBar CPU-Issue workaround
- `0ed9675` [Wallet] Add global boolean whether to send free transactions (default=true)
- `ed3e5e4` [Wallet] Add global boolean whether to pay at least the custom fee (default=true)
- `e7876b2` [Wallet] Prevent user from paying a non-sense fee
- `c1c9d5b` Add Smartfee to GUI
- `e0a25c5` Make askpassphrase dialog behave more sanely
- `94b362d` On close of splashscreen interrupt verifyDB
- `b790d13` English translation update
- `8543b0d` Correct tooltip on address book page
- `2c08406` some mac specifiy cleanup (memory handling, unnecessary code)
- `81145a6` fix OSX dock icon window reopening
- `786cf72` fix a issue where "command line options"-action overwrite "Preference"-action (on OSX)

Tests:
- `b41e594` Fix script test handling of empty scripts
- `d3a33fc` Test CHECKMULTISIG with m == 0 and n == 0
- `29c1749` Let tx (in)valid tests use any SCRIPT_VERIFY flag
- `6380180` Add rejection of non-null CHECKMULTISIG dummy values
- `21bf3d2` Add tests for BoostAsioToCNetAddr
- `b5ad5e7` Add Python test for -rpcbind and -rpcallowip
- `9ec0306` Add CODESEPARATOR/FindAndDelete() tests
- `75ebced` Added many rpc wallet tests
- `0193fb8` Allow multiple regression tests to run at once
- `92a6220` Hook up sanity checks
- `3820e01` Extend and move all crypto tests to crypto_tests.cpp
- `3f9a019` added list/get received by address/ account tests
- `a90689f` Remove timing-based signature cache unit test
- `236982c` Add skiplist unit tests
- `f4b00be` Add CChain::GetLocator() unit test
- `b45a6e8` Add test for getblocktemplate longpolling
- `cdf305e` Set -discover=0 in regtest framework
- `ed02282` additional test for OP_SIZE in script_valid.json
- `0072d98` script tests: BOOLAND, BOOLOR decode to integer
- `833ff16` script tests: values that overflow to 0 are true
- `4cac5db` script tests: value with trailing 0x00 is true
- `89101c6` script test: test case for 5-byte bools
- `d2d9dc0` script tests: add tests for CHECKMULTISIG limits
- `d789386` Add "it works" test for bitcoin-tx
- `df4d61e` Add bitcoin-tx tests
- `aa41ac2` Test IsPushOnly() with invalid push
- `6022b5d` Make `script_{valid,invalid}.json` validation flags configurable
- `8138cbe` Add automatic script test generation, and actual checksig tests
- `ed27e53` Add coins_tests with a large randomized CCoinViewCache test
- `9df9cf5` Make SCRIPT_VERIFY_STRICTENC compatible with BIP62
- `dcb9846` Extend getchaintips RPC test
- `554147a` Ensure MINIMALDATA invalid tests can only fail one way
- `dfeec18` Test every numeric-accepting opcode for correct handling of the numeric minimal encoding rule
- `2b62e17` Clearly separate PUSHDATA and numeric argument MINIMALDATA tests
- `16d78bd` Add valid invert of invalid every numeric opcode tests
- `f635269` tests: enable alertnotify test for Windows
- `7a41614` tests: allow rpc-tests to get filenames for bitcoind and bitcoin-cli from the environment
- `5122ea7` tests: fix forknotify.py on windows
- `fa7f8cd` tests: remove old pull-tester scripts
- `7667850` tests: replace the old (unused since Travis) tests with new rpc test scripts
- `f4e0aef` Do signature-s negation inside the tests
- `1837987` Optimize -regtest setgenerate block generation
- `2db4c8a` Fix node ranges in the test framework
- `a8b2ce5` regression test only setmocktime RPC call
- `daf03e7` RPC tests: create initial chain with specific timestamps
- `8656dbb` Port/fix txnmall.sh regression test
- `ca81587` Test the exact order of CHECKMULTISIG sig/pubkey evaluation
- `7357893` Prioritize and display -testsafemode status in UI
- `f321d6b` Add key generation/verification to ECC sanity check
- `132ea9b` miner_tests: Disable checkpoints so they don't fail the subsidy-change test
- `bc6cb41` QA RPC tests: Add tests block block proposals
- `f67a9ce` Use deterministically generated script tests
- `11d7a7d` [RPC] add rpc-test for http keep-alive (persistent connections)
- `34318d7` RPC-test based on invalidateblock for mempool coinbase spends
- `76ec867` Use actually valid transactions for script tests
- `c8589bf` Add actual signature tests
- `e2677d7` Fix smartfees test for change to relay policy
- `263b65e` tests: run sanity checks in tests too
- `1117378` add RPC test for InvalidateBlock

Miscellaneous:
- `122549f` Fix incorrect checkpoint data for testnet3
- `5bd02cf` Log used config file to debug.log on startup
- `68ba85f` Updated Debian example bitcoin.conf with config from wiki + removed some cruft and updated comments
- `e5ee8f0` Remove -beta suffix
- `38405ac` Add comment regarding experimental-use service bits
- `be873f6` Issue warning if collecting RandSeed data failed
- `8ae973c` Allocate more space if necessary in RandSeedAddPerfMon
- `675bcd5` Correct comment for 15-of-15 p2sh script size
- `fda3fed` libsecp256k1 integration
- `2e36866` Show nodeid instead of addresses in log (for anonymity) unless otherwise requested
- `cd01a5e` Enable paranoid corruption checks in LevelDB >= 1.16
- `9365937` Add comment about never updating nTimeOffset past 199 samples
- `403c1bf` contrib: remove getwork-based pyminer (as getwork API call has been removed)
- `0c3e101` contrib: Added systemd .service file in order to help distributions integrate bitcoind
- `0a0878d` doc: Add new DNSseed policy
- `2887bff` Update coding style and add .clang-format
- `5cbda4f` Changed LevelDB cursors to use scoped pointers to ensure destruction when going out of scope
- `b4a72a7` contrib/linearize: split output files based on new-timestamp-year or max-file-size
- `e982b57` Use explicit fflush() instead of setvbuf()
- `234bfbf` contrib: Add init scripts and docs for Upstart and OpenRC
- `01c2807` Add warning about the merkle-tree algorithm duplicate txid flaw
- `d6712db` Also create pid file in non-daemon mode
- `772ab0e` contrib: use batched JSON-RPC in linarize-hashes (optimization)
- `7ab4358` Update bash-completion for v0.10
- `6e6a36c` contrib: show pull # in prompt for github-merge script
- `5b9f842` Upgrade leveldb to 1.18, make chainstate databases compatible between ARM and x86 (issue #2293)
- `4e7c219` Catch UTXO set read errors and shutdown
- `867c600` Catch LevelDB errors during flush
- `06ca065` Fix CScriptID(const CScript& in) in empty script case
- `c9e022b` Initialization: set Boost path locale in main thread
- `23126a0` Sanitize command strings before logging them.
- `323de27` Initialization: setup environment before starting Qt tests
- `7494e09` Initialization: setup environment before starting tests
- `df45564` Initialization: set fallback locale as environment variable
- `da65606` Avoid crash on start in TestBlockValidity with gen=1.
- `424ae66` don't imbue boost::filesystem::path with locale "C" on windows (fixes #6078)

For convenience in locating the code changes and accompanying
discussion, the following changes reference both the pull request and
git merge commit.

Bitcoin Core pull requests
- #6186 `e4a7d51` Fix two problems in CSubnet parsing
- #6153 `ebd7d8d` Parameter interaction: disable upnp if -proxy set
- #6203 `ecc96f5` Remove P2SH coinbase flag, no longer interesting
- #6226 `181771b` json: fail read_string if string contains trailing garbage
- #6244 `09334e0` configure: Detect (and reject) LibreSSL
- #6276 `0fd8464` Fix getbalance * 0
- #6274 `be64204` Add option `-alerts` to opt out of alert system
- #6319 `3f55638` doc: update mailing list address
- #6438 `7e66e9c` openssl: avoid config file load/race
- #6439 `255eced` Updated URL location of netinstall for Debian
- #6412 `0739e6e` Test whether created sockets are select()able
- #6694 `f696ea1` [QT] fix thin space word wrap line brake issue
- #6704 `743cc9e` Backport bugfixes to 0.10
- #6769 `1cea6b0` Test LowS in standardness, removes nuisance malleability vector.
- #6789 `093d7b5` Update miniupnpc to 1.9.20151008
- #6795 `f2778e0` net: Disable upnp by default
- #6797 `91ef4d9` Do not store more than 200 timedata samples
- #6793 `842c48d` Bump minrelaytxfee default
- #6953 `8b3311f` alias -h for --help
- #6953 `97546fc` Change URLs to https in debian/control
- #6953 `38671bf` Update debian/changelog and slight tweak to debian/control
- #6953 `256321e` Correct spelling mistakes in doc folder
- #6953 `eae0350` Clarification of unit test build instructions
- #6953 `90897ab` Update bluematt-key, the old one is long-since revoked
- #6953 `a2f2fb6` build: disable -Wself-assign
- #6953 `cf67d8b` Bugfix: Allow mining on top of old tip blocks for testnet (fixes testnet-in-a-box use case)
- #6953 `b3964e3` Drop "with minimal dependencies" from description
- #6953 `43c2789` Split freicoin-tx into its own package
- #6953 `dfe0d4d` Include freicoin-tx binary on Debian/Ubuntu
- #6953 `612efe8` [Qt] Raise debug window when requested
- #6953 `3ad96bd` Fix locking in GetTransaction
- #6953 `9c81005` Fix spelling of Qt
- #6946 `94b67e5` Update LevelDB
- #6706 `5dc72f8` CLTV: Add more tests to improve coverage
- #6706 `6a1343b` Add RPC tests for the CHECKLOCKTIMEVERIFY (BIP65) soft-fork
- #6706 `4137248` Add CHECKLOCKTIMEVERIFY (BIP65) soft-fork logic
- #6706 `0e01d0f` Enable CHECKLOCKTIMEVERIFY as a standard script verify flag
- #6706 `6d01325` Replace NOP2 with CHECKLOCKTIMEVERIFY (BIP65)
- #6706 `750d54f` Move LOCKTIME_THRESHOLD to src/script/script.h
- #6706 `6897468` Make CScriptNum() take nMaxNumSize as an argument
- #6867 `5297194` Set TCP_NODELAY on P2P sockets
- #6836 `fb818b6` Bring historical release notes up to date
- #6852 `0b3fd07` build: make sure OpenSSL heeds noexecstack

Tradecraft pull requests:

- #23 `9f12779` [Demurrage] Add inverse-demurrage calculations,
   permitting GetTimeAdjustedValue to be called with negative
   relative_depth values and yield reasonable results.

- #23 `6ca19e0` [Alert] Do not warn that an upgrade is required when
   unrecognized block versions have been observed.

- #24 `2289825` [Branding] Update macOS icon file to use new Freicoin
   kria logo.

- #26 `8333769` [Hard-Fork] Implement time-activated "protocol
   cleanup" flag-day hard-fork, set to activate in April 2021.

- #28 `d9b93a4` [Standard] Remove SCRIPT_VERIFY_NULLDUMMY from the
   standard verification flags.

- #29 `62ce0e7` [Soft-fork] Require the nTimeLock value of a coinbase
   transaction to be equal to the median of the past 11 block times.

Credits
=======

Thanks to everyone who contributed to this release:

- 21E14
- Adam Weiss
- Aitor Pazos
- Alex Morcos
- Alexander Jeng
- Alon Muroch
- Andreas Schildbach
- Andrew Poelstra
- Andy Alness
- Ashley Holman
- Ben Holden-Crowther
- Benedict Chan
- Bryan Bishop
- BtcDrak
- Casey Rodarmor
- Christian von Roques
- Clinton Christian
- Cory Fields
- Cozz Lovan
- daniel
- Daniel Cousens
- Daniel Kraft
- David Hill
- Derek701
- dexX7
- Diego Viola
- dllud
- Dominyk Tiller
- Doug
- elichai
- elkingtowa
- ENikS
- Eric Lombrozo
- Eric Shaw
- Esteban Ordano
- fanquake
- Federico Bond
- Francis GASCHET
- Fredrik Bodin
- fsb4000
- Gavin Andresen
- Giuseppe Mazzotta
- Glenn Willen
- Gregory Maxwell
- gubatron
- HarryWu
- himynameismartin
- Huang Le
- Ian Carroll
- imharrywu
- Ivan Pustogarov
- J Ross Nicoll
- Jameson Lopp
- Janusz Lenar
- JaSK
- Jeff Garzik
- JL2035
- Johnathan Corgan
- Jonas Schnelli
- jtimon
- Julian Haight
- Kamil Domanski
- Karl Johan Alm
- kazcw
- kevin
- kiwigb
- Kosta Zertsekel
- LongShao007
- Luke Dashjr
- MarcoFalke
- Mark Friedenbach
- Mathy Vanvoorden
- Matt Corallo
- Matthew Bogosian
- Micha
- Michael Ford
- Mike Hearn
- Mitchell Cash
- mrbandrews
- mruddy
- ntrgn
- Otto Allmendinger
- Pavel Vasin
- paveljanik
- Peter Todd
- phantomcircuit
- Philip Kaufmann
- Pieter Wuille
- pryds
- R E Broadley
- randy-waterhouse
- Rose Toomey
- Ross Nicoll
- Roy Badami
- Ruben Dario Ponticelli
- Ruben de Vries
- Rune K. Svendsen
- Ryan X. Charles
- Saivann
- sandakersmann
- SergioDemianLerner
- shshshsh
- sinetek
- Stuart Cardall
- Suhas Daftuar
- Tawanda Kembo
- Teran McKinney
- tm314159
- Tom Harding
- Trevin Hofmann
- Veres Lajos
- Whit J
- Wladimir J. van der Laan
- Yoichi Hirai
- Zak Wilcox
- ฿tcDrak

And all those who contributed additional code review and/or security research:

- 21E14
- Alison Kendler
- Aviv Zohar
- dexX7
- Ethan Heilman
- Evil-Knievel
- fanquake
- Fredrik Bodin
- Jeff Garzik
- Jonas Nick
- Luke Dashjr
- Patrick Strateman
- Philip Kaufmann
- Pieter Wuille
- Sergio Demian Lerner
- Sharon Goldberg
- timothy on IRC for reporting the issue
- vayvanne
- Vulnerability in miniupnp discovered by Aleksandar Nikolic of Cisco Talos

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).

