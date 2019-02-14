v21.2-26860 Release Notes
=========================

Freicoin version v21.2-26860 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v21.2-26860

This release includes new features, various bug fixes and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

How to Upgrade
--------------

If you are running an older version, shut it down.  Wait until it has completely shut down (which might take a few minutes in some cases), then run the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on Mac) or `freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is possible, but it might take some time if the data directory needs to be migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
-------------

Freicoin is supported and extensively tested on operating systems using the Linux kernel, macOS 10.12+, and Windows 7 and newer.  Freicoin should also work on most other Unix-like systems but is not as frequently tested on them.  It is not recommended to use Freicoin on unsupported systems.

From Freicoin v20 onwards, macOS versions earlier than 10.12 are no longer supported.  Additionally, Freicoin does not yet change appearance when macOS "dark mode" is activated.

The node's known peers are persisted to disk in a file called `peers.dat`.  The format of this file has been changed in a backwards-incompatible way in order to accommodate the storage of Tor v3 and other BIP155 addresses.  This means that if the file is modified by v21.2-26860 or newer then older versions will not be able to read it.  Those old versions, in the event of a downgrade, will log an error message "Incorrect keysize in addrman deserialization" and will continue normal operation as if the file was missing, creating a new empty one. (bitcoin/bitcoin#19954, bitcoin/bitcoin#20284)

Notable changes
---------------

### P2P and network changes

- The mempool now tracks whether transactions submitted via the wallet or RPCs have been successfully broadcast.  Every 10-15 minutes, the node will try to announce unbroadcast transactions until a peer requests it via a `getdata` message or the transaction is removed from the mempool for other reasons.  The node will not track the broadcast status of transactions submitted to the node using P2P relay.  This version reduces the initial broadcast guarantees for wallet transactions submitted via P2P to a node running the wallet.  (bitcoin/bitcoin#18038)

- The size of the set of transactions that peers have announced and we consider for requests has been reduced from 100000 to 5000 (per peer), and further announcements will be ignored when that limit is reached.  If you need to dump (very) large batches of transactions, exceptions can be made for trusted peers using the "relay" network permission. For localhost for example it can be enabled using the command line option `-whitelist=relay@127.0.0.1`.  (bitcoin/bitcoin#19988)

- This release adds support for Tor version 3 hidden services, and rumoring them over the network to other peers using [BIP155](https://github.com/bitcoin/bips/blob/master/bip-0155.mediawiki).  Version 2 hidden services are still fully supported by Freicoin, but the Tor network will start [deprecating](https://blog.torproject.org/v2-deprecation-timeline) them in the coming months.  (bitcoin/bitcoin#19954)

- The Tor onion service that is automatically created by setting the `-listenonion` configuration parameter will now be created as a Tor v3 service instead of Tor v2.  The private key that was used for Tor v2 (if any) will be left untouched in the `onion_private_key` file in the data directory (see `-datadir`) and can be removed if not needed.  Freicoin will no longer attempt to read it.  The private key for the Tor v3 service will be saved in a file named `onion_v3_private_key`.  To use the deprecated Tor v2 service (not recommended), the `onion_private_key` can be copied over `onion_v3_private_key`, e.g.  `cp -f onion_private_key onion_v3_private_key`.  (bitcoin/bitcoin#19954)

- The client writes a file (`anchors.dat`) at shutdown with the network addresses of the node’s two outbound block-relay-only peers (so called "anchors").  The next time the node starts, it reads this file and attempts to reconnect to those same two peers.  This prevents an attacker from using node restarts to trigger a complete change in peers, which would be something they could use as part of an eclipse attack.  (bitcoin/bitcoin#17428)

- This release adds support for serving [BIP157](https://github.com/bitcoin/bips/blob/master/bip-0157.mediawiki) compact filters to peers on the network when enabled using `-blockfilterindex=1 -peerblockfilters=1`.  (bitcoin/bitcoin#16442)

- This release implements [BIP339](https://github.com/bitcoin/bips/blob/master/bip-0339.mediawiki) wtxid relay.  When negotiated, transactions are announced using their wtxid instead of their txid.  (bitcoin/bitcoin#18044)

### Updated RPCs

- The `getpeerinfo` RPC has a new `network` field that provides the type of network ("ipv4", "ipv6", or "onion") that the peer connected through.  (bitcoin/bitcoin#20002)

- The `getpeerinfo` RPC now has additional `last_block` and `last_transaction` fields that return the UNIX epoch time of the last block and the last *valid* transaction received from each peer.  (bitcoin/bitcoin#19731)

- `getnetworkinfo` now returns two new fields, `connections_in` and `connections_out`, that provide the number of inbound and outbound peer connections. These new fields are in addition to the existing `connections` field, which returns the total number of peer connections. (bitcoin/bitcoin#19405)

- Exposed transaction version numbers are now treated as unsigned 32-bit integers instead of signed 32-bit integers.  This matches their treatment in consensus logic.  Versions greater than 2 continue to be non-standard (matching previous behavior of smaller than 1 or greater than 2 being non-standard).  Note that this includes the `joinpsbt` command, which combines partially-signed transactions by selecting the highest version number.  (bitcoin/bitcoin#16525)

- `getmempoolinfo` now returns an additional `unbroadcastcount` field.  The mempool tracks locally submitted transactions until their initial broadcast is acknowledged by a peer.  This field returns the count of transactions waiting for acknowledgement.

- Mempool RPCs such as `getmempoolentry` and `getrawmempool` with `verbose=true` now return an additional `unbroadcast` field.  This indicates whether initial broadcast of the transaction has been acknowledged by a peer.  `getmempoolancestors` and `getmempooldescendants` are also updated.

- The `getpeerinfo` RPC no longer returns the `banscore` field unless the configuration option `-deprecatedrpc=banscore` is used.  The `banscore` field will be fully removed in the next major release.  (bitcoin/bitcoin#19469)

- The `testmempoolaccept` RPC returns `vsize` and a `fees` object with the `base` fee if the transaction would pass validation.  (bitcoin/bitcoin#19940)

- The `getpeerinfo` RPC now returns a `connection_type` field.  This indicates the type of connection established with the peer.  It will return one of six options.  For more information, see the `getpeerinfo` help documentation.  (bitcoin/bitcoin#19725)

- The `getpeerinfo` RPC no longer returns the `addnode` field by default.  This field will be fully removed in the next major release.  It can be accessed with the configuration option `-deprecatedrpc=getpeerinfo_addnode`.  However, it is recommended to instead use the `connection_type` field (it will return `manual` when addnode is true).  (bitcoin/bitcoin#19725)

- The `getpeerinfo` RPC no longer returns the `whitelisted` field by default. This field will be fully removed in the next major release.  It can be accessed with the configuration option `-deprecatedrpc=getpeerinfo_whitelisted`.  However, it is recommended to instead use the `permissions` field to understand if specific privileges have been granted to the peer.  (bitcoin/bitcoin#19770)

- The `walletcreatefundedpsbt` RPC call will now fail with `Insufficient funds` when inputs are manually selected but are not enough to cover the outputs and fee.  Additional inputs can automatically be added through the new `add_inputs` option.  (bitcoin/bitcoin#16377)

- The `fundrawtransaction` RPC now supports `add_inputs` option that when `false` prevents adding more inputs if necessary and consequently the RPC fails.

Changes to Wallet or GUI related RPCs can be found in the GUI or Wallet section below.

### New RPCs

- The `getindexinfo` RPC returns the actively running indices of the node, including their current sync status and height.  It also accepts an `index_name` to specify returning the status of that index only.  (bitcoin/bitcoin#19550)

### Updated settings

- The same ZeroMQ notification (e.g. `-zmqpubhashtx=address`) can now be specified multiple times to publish the same notification to different ZeroMQ sockets.  (bitcoin/bitcoin#18309)

- The `-banscore` configuration option, which modified the default threshold for disconnecting and discouraging misbehaving peers, has been removed as part of changes in this release to the handling of misbehaving peers.  (bitcoin/bitcoin#19464)

- The `-debug=db` logging category, which was deprecated in v20 and replaced by `-debug=walletdb` to distinguish it from `coindb`, has been removed.  (bitcoin/bitcoin#19202)

- A `download` permission has been extracted from the `noban` permission.  For compatibility, `noban` implies the `download` permission, but this may change in future releases.  Refer to the help of the affected settings `-whitebind` and `-whitelist` for more details.  (bitcoin/bitcoin#19191)

- Netmasks that contain 1-bits after 0-bits (the 1-bits are not contiguous on the left side, e.g. 255.0.255.255) are no longer accepted.  They are invalid according to RFC 4632.  Netmasks are used in the `-rpcallowip` and `-whitelist` configuration options and in the `setban` RPC.  (bitcoin/bitcoin#19628)

- The `-blocksonly` setting now completely disables fee estimation.  (bitcoin/bitcoin#18766)

Changes to Wallet or GUI related settings can be found in the GUI or Wallet section below.

### Tools and Utilities

- A new `freicoin-cli -netinfo` command provides a network peer connections dashboard that displays data from the `getpeerinfo` and `getnetworkinfo` RPCs in a human-readable format.  An optional integer argument from `0` to `4` may be passed to see increasing levels of detail.  (bitcoin/bitcoin#19643)

- A new `freicoin-cli -generate` command, equivalent to RPC `generatenewaddress` followed by `generatetoaddress`, can generate blocks for command line testing purposes.  This is a client-side version of the former `generate` RPC. See the help for details.  (bitcoin/bitcoin#19133)

- The `freicoin-cli -getinfo` command now displays the wallet name and balance for each of the loaded wallets when more than one is loaded (e.g. in multiwallet mode) and a wallet is not specified with `-rpcwallet`.  (bitcoin/bitcoin#18594)

- The `connections` field of `freicoin-cli -getinfo` is now expanded to return a JSON object with `in`, `out` and `total` numbers of peer connections.  It previously returned a single integer value for the total number of peer connections.  (bitcoin/bitcoin#19405)

### New settings

- The `startupnotify` option is used to specify a command to execute when Freicoin has finished with its startup sequence.  (bitcoin/bitcoin#15367)

### Wallet

- Backwards compatibility has been dropped for two `getaddressinfo` RPC deprecations, as notified in the v20 release notes.  The deprecated `label` field has been removed as well as the deprecated `labels` behavior of returning a JSON object containing `name` and `purpose` key-value pairs.  Since v20, the `labels` field returns a JSON array of label names.  (bitcoin/bitcoin#19200)

- To improve wallet privacy, the frequency of wallet rebroadcast attempts is reduced from approximately once every 15 minutes to once every 12-36 hours.  To maintain a similar level of guarantee for initial broadcast of wallet transactions, the mempool tracks these transactions as a part of the newly introduced unbroadcast set.  See the "P2P and network changes" section for more information on the unbroadcast set.  (bitcoin/bitcoin#18038)

- The `sendtoaddress` and `sendmany` RPCs accept an optional `verbose=True` argument to also return the fee reason about the sent tx.  (bitcoin/bitcoin#19501)

- The wallet can create a transaction without change even when the keypool is empty.  Previously it failed.  (bitcoin/bitcoin#17219)

- The `-salvagewallet` startup option has been removed.  A new `salvage` command has been added to the `freicoin-wallet` tool which performs the salvage operations that `-salvagewallet` did.  (bitcoin/bitcoin#18918)

- A new configuration flag `-maxapsfee` has been added, which sets the max allowed avoid partial spends (APS) fee.  It defaults to 0 (i.e. fee is the same with and without APS).  Setting it to -1 will disable APS, unless `-avoidpartialspends` is set.  (bitcoin/bitcoin#14582)

- The wallet will now avoid partial spends (APS) by default, if this does not result in a difference in fees compared to the non-APS variant.  The allowed fee threshold can be adjusted using the new `-maxapsfee` configuration option.  (bitcoin/bitcoin#14582)

- The `createwallet`, `loadwallet`, and `unloadwallet` RPCs now accept `load_on_startup` options to modify the settings list.  Unless these options are explicitly set to true or false, the list is not modified, so the RPC methods remain backwards compatible.  (bitcoin/bitcoin#15937)

- A new `send` RPC with similar syntax to `walletcreatefundedpsbt`, including support for coin selection and a custom fee rate, is added. The `send` RPC is experimental and may change in subsequent releases.  (bitcoin/bitcoin#16378)

- The `estimate_mode` parameter is now case-insensitive in the `bumpfee`, `fundrawtransaction`, `sendmany`, `sendtoaddress`, `send` and `walletcreatefundedpsbt` RPCs.  (bitcoin/bitcoin#11413)

- The `bumpfee` RPC now uses `conf_target` rather than `confTarget` in the options.  (bitcoin/bitcoin#11413)

- `fundrawtransaction` and `walletcreatefundedpsbt` when used with the `lockUnspents` argument now lock manually selected coins, in addition to automatically selected coins.  Note that locked coins are never used in automatic coin selection, but can still be manually selected.  (bitcoin/bitcoin#18244)

- The `-zapwallettxes` startup option has been removed and its functionality removed from the wallet.  This option was originally intended to allow for rescuing wallets which were affected by a malleability attack.  More recently, it has been used in the fee bumping of transactions that did not signal RBF.  This functionality has been superseded with the abandon transaction feature.  (bitcoin/bitcoin#19671)

- The error code when no wallet is loaded, but a wallet RPC is called, has been changed from `-32601` (method not found) to `-18` (wallet not found).  (bitcoin/bitcoin#20101)

##### Automatic wallet creation removed

Freicoin will no longer automatically create new wallets on startup.  It will load existing wallets specified by `-wallet` options on the command line or in `freicoin.conf` or `settings.json` files. And by default it will also load a top-level unnamed ("") wallet.  However, if specified wallets don't exist, Freicoin will now just log warnings instead of creating new wallets with new keys and addresses like previous releases did.

New wallets can be created through the GUI (which has a more prominent create wallet option), through the `freicoin-cli createwallet` or `freicoin-wallet create` commands, or the `createwallet` RPC.  (bitcoin/bitcoin#15454, bitcoin/bitcoin#20186)

##### Experimental Descriptor Wallets

Please note that Descriptor Wallets are still experimental and not all expected functionality is available.  Additionally there may be some bugs and current functions may change in the future.  Bugs and missing functionality can be reported to the [issue tracker](https://github.com/tradecraftio/tradecraft/issues).

v21 introduces a new type of wallet - Descriptor Wallets.  Descriptor Wallets store scriptPubKey information using output descriptors.  This is in contrast to the Legacy Wallet structure where keys are used to implicitly generate scriptPubKeys and addresses.  Because of this shift to being script based instead of key based, many of the confusing things that Legacy Wallets do are not possible with Descriptor Wallets.  Descriptor Wallets use a definition of "mine" for scripts which is simpler and more intuitive than that used by Legacy Wallets.  Descriptor Wallets also uses different semantics for watch-only things and imports.

As Descriptor Wallets are a new type of wallet, their introduction does not affect existing wallets.  Users who already have a Freicoin wallet can continue to use it as they did before without any change in behavior.  Newly created Legacy Wallets (which remains the default type of wallet) will behave as they did in previous versions of Freicoin.

The differences between Descriptor Wallets and Legacy Wallets are largely limited to non user facing things.  They are intended to behave similarly except for the import/export and watchonly functionality as described below.

###### Creating Descriptor Wallets

Descriptor wallets are not the default type of wallet.

In the GUI, a checkbox has been added to the Create Wallet Dialog to indicate that a Descriptor Wallet should be created.  And a `descriptors` option has been added to `createwallet` RPC.  Setting `descriptors` to `true` will create a Descriptor Wallet instead of a Legacy Wallet.

Without those options being set, a Legacy Wallet will be created instead.

###### `IsMine` Semantics

`IsMine` refers to the function used to determine whether a script belongs to the wallet.  This is used to determine whether an output belongs to the wallet.  `IsMine` in Legacy Wallets returns true if the wallet would be able to sign an input that spends an output with that script.  Since keys can be involved in a variety of different scripts, this definition for `IsMine` can lead to many unexpected scripts being considered part of the wallet.

With Descriptor Wallets, descriptors explicitly specify the set of scripts that are owned by the wallet.  Since descriptors are deterministic and easily enumerable, users will know exactly what scripts the wallet will consider to belong to it.  Additionally the implementation of `IsMine` in Descriptor Wallets is far simpler than for Legacy Wallets.  Notably, in Legacy Wallets, `IsMine` allowed for users to take one type of address (e.g. P2PKH), mutate it into another address type (e.g. P2WPKH), and the wallet would still detect outputs sending to the new address type even without that address being requested from the wallet.  Descriptor Wallets do not allow for this and will only watch for the addresses that were explicitly requested from the wallet.

These changes to `IsMine` will make it easier to reason about what scripts the wallet will actually be watching for in outputs. However for the vast majority of users, this change is largely transparent and will not have noticeable effect.

##### Imports and Exports

In Legacy Wallets, raw scripts and keys could be imported to the wallet.  Those imported scripts and keys are treated separately from the keys generated by the wallet.  This complicates the `IsMine` logic as it has to distinguish between spendable and watchonly.

Descriptor Wallets handle importing scripts and keys differently.  Only complete descriptors can be imported.  These descriptors are then added to the wallet as if it were a descriptor generated by the wallet itself.  This simplifies the `IsMine` logic so that it no longer has to distinguish between spendable and watchonly.  As such, the watchonly model for Descriptor Wallets is also different and described in more detail in the next section.

To import into a Descriptor Wallet, a new `importdescriptors` RPC has been added that uses a syntax similar to that of `importmulti`.

As Legacy Wallets and Descriptor Wallets use different mechanisms for storing and importing scripts and keys the existing import RPCs have been disabled for descriptor wallets.  New export RPCs for Descriptor Wallets have not yet been added.

The following RPCs are disabled for Descriptor Wallets:

* `importprivkey`
* `importpubkey`
* `importaddress`
* `importwallet`
* `dumpprivkey`
* `dumpwallet`
* `importmulti`
* `addmultisigaddress`
* `sethdseed`

##### Watchonly Wallets

A Legacy Wallet contains both private keys and scripts that were being watched.  Those watched scripts would not contribute to your normal balance.  In order to see the watchonly balance and to use watchonly things in transactions, an `include_watchonly` option was added to many RPCs that would allow users to do that.  However it is easy to forget to include this option.

Descriptor Wallets move to a per-wallet watchonly model.  Instead an entire wallet is considered to be watchonly depending on whether it was created with private keys disabled.  This eliminates the need to distinguish between things that are watchonly and things that are not within a wallet itself.

This change does have a caveat.  If a Descriptor Wallet with private keys *enabled* has a multiple key descriptor without all of the private keys (e.g. `multi(...)` with only one private key), then the wallet will fail to sign and broadcast transactions.  Such wallets would need to use the PSBT workflow but the typical GUI Send, `sendtoaddress`, etc. workflows would still be available, just non-functional.

This issue is worsened if the wallet contains both single key (e.g. `wpkh(...)`) descriptors and such multiple key descriptors as some transactions could be signed and broadcast and others not.  This is due to some transactions containing only single key inputs, while others would contain both single key and multiple key inputs, depending on which are available and how the coin selection algorithm selects inputs.  However this is not considered to be a supported use case; multisigs should be in their own wallets which do not already have descriptors.  Although users cannot export descriptors with private keys for now as explained earlier.

##### BIP 44/49/84 Support

The change to using descriptors changes the default derivation paths used by Freicoin to adhere to BIP 44/49/84.  Descriptors with different derivation paths can be imported without issue.

##### SQLite Database Backend

Descriptor wallets use SQLite for the wallet file instead of the Berkeley DB used in legacy wallets.  This will break compatibility with any existing tooling that operates on wallets, however compatibility was already being broken by the move to descriptors.

##### Wallet RPC changes

- The `upgradewallet` RPC replaces the `-upgradewallet` command line option.  (bitcoin/bitcoin#15761)

- The `settxfee` RPC will fail if the fee was set higher than the `-maxtxfee` command line setting.  The wallet will already fail to create transactions with fees higher than `-maxtxfee`.  (bitcoin/bitcoin#18467)

- A new `fee_rate` parameter/option denominated in kria per vbyte (sat/vB) is introduced to the `sendtoaddress`, `sendmany`, `fundrawtransaction` and `walletcreatefundedpsbt` RPCs as well as to the experimental new `send` RPC.  The legacy `feeRate` option in `fundrawtransaction` and `walletcreatefundedpsbt` still exists for setting a fee rate in FRC per 1,000 vbytes (FRC/kvB), but it is expected to be deprecated soon to avoid confusion.  For these RPCs, the fee rate error message is updated from FRC/kB to sat/vB and the help documentation in FRC/kB is updated to FRC/kvB.  The `send` and `sendtoaddress` RPC examples are updated to aid users in creating transactions with explicit fee rates.  (bitcoin/bitcoin#20305, bitcoin/bitcoin#11413)

- The `bumpfee` RPC `fee_rate` option is changed from FRC/kvB to sat/vB and the help documentation is updated.  Users are warned that this is a breaking API change, but it should be relatively benign: the large (100,000 times) difference between FRC/kvB and sat/vB units means that a transaction with a fee rate mistakenly calculated in FRC/kvB rather than sat/vB should raise an error due to the fee rate being set too low.  In the worst case, the transaction may send at 1 sat/vB, but as Replace-by-Fee (BIP125 RBF) is active by default when an explicit fee rate is used, the transaction fee can be bumped.  (bitcoin/bitcoin#20305)

### GUI changes

- Wallets created or loaded in the GUI will now be automatically loaded on startup, so they don't need to be manually reloaded next time Freicoin is started.  The list of wallets to load on startup is stored in `\<datadir\>/settings.json` and augments any command line or `freicoin.conf` `-wallet=` settings that specify more wallets to load.  Wallets that are unloaded in the GUI get removed from the settings list so they won't load again automatically next startup.  (bitcoin/bitcoin#19754)

- The GUI Peers window no longer displays a "Ban Score" field.  (bitcoin/bitcoin#19512)

Low-level changes
-----------------

### RPC

- To make RPC `sendtoaddress` more consistent with `sendmany` the following error `sendtoaddress` codes were changed from `-4` to `-6`:
  - Insufficient funds
  - Fee estimation failed
  - Transaction has too long of a mempool chain

- The `sendrawtransaction` error code for exceeding `maxfeerate` has been changed from `-26` to `-25`.  The error string has been changed from "absurdly-high-fee" to "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)." The `testmempoolaccept` RPC returns `max-fee-exceeded` rather than `absurdly-high-fee` as the `reject-reason`.  (bitcoin/bitcoin#19339)

- To make wallet and rawtransaction RPCs more consistent, the error message for exceeding maximum feerate has been changed to "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)."  (bitcoin/bitcoin#19339)

### Tests

- The `generateblock` RPC allows testers using regtest mode to generate blocks that consist of a custom set of transactions.  (bitcoin/bitcoin#17693)

v21.2-26860 Change log
----------------------

### Consensus
- bitcoin/bitcoin#18267 BIP-325: Signet (kallewoof)
- bitcoin/bitcoin#20016 uint256: 1 is a constant (ajtowns)
- bitcoin/bitcoin#20006 Fix misleading error message: Clean stack rule (sanket1729)
- bitcoin/bitcoin#19953 Implement BIP 340-342 validation (Schnorr/taproot/tapscript) (sipa)
- bitcoin/bitcoin#20169 Taproot follow-up: Make ComputeEntrySchnorr and ComputeEntryECDSA const to clarify contract (practicalswift)
- bitcoin/bitcoin#21377 Speedy trial support for versionbits (ajtowns)
- bitcoin/bitcoin#21686 Speedy trial activation parameters for Taproot (achow101)

### Policy
- bitcoin/bitcoin#18766 Disable fee estimation in blocksonly mode (darosior)
- bitcoin/bitcoin#19630 Cleanup fee estimation code (darosior)
- bitcoin/bitcoin#20165 Only relay Taproot spends if next block has it active (sipa)

### Mining
- bitcoin/bitcoin#17946 Fix GBT: Restore "!segwit" and "csv" to "rules" key (luke-jr)

### Privacy
- bitcoin/bitcoin#16432 Add privacy to the Overview page (hebasto)
- bitcoin/bitcoin#18861 Do not answer GETDATA for to-be-announced tx (sipa)
- bitcoin/bitcoin#18038 Mempool tracks locally submitted transactions to improve wallet privacy (amitiuttarwar)
- bitcoin/bitcoin#19109 Only allow getdata of recently announced invs (sipa)

### Block and transaction handling
- bitcoin/bitcoin#17737 Add ChainstateManager, remove BlockManager global (jamesob)
- bitcoin/bitcoin#18960 indexes: Add compact block filter headers cache (jnewbery)
- bitcoin/bitcoin#13204 Faster sigcache nonce (JeremyRubin)
- bitcoin/bitcoin#19088 Use std::chrono throughout some validation functions (fanquake)
- bitcoin/bitcoin#19142 Make VerifyDB level 4 interruptible (MarcoFalke)
- bitcoin/bitcoin#17994 Flush undo files after last block write (kallewoof)
- bitcoin/bitcoin#18990 log: Properly log txs rejected from mempool (MarcoFalke)
- bitcoin/bitcoin#18984 Remove unnecessary input blockfile SetPos (dgenr8)
- bitcoin/bitcoin#19526 log: Avoid treating remote misbehvior as local system error (MarcoFalke)
- bitcoin/bitcoin#18044 Use wtxid for transaction relay (sdaftuar)
- bitcoin/bitcoin#18637 coins: allow cache resize after init (jamesob)
- bitcoin/bitcoin#19854 Avoid locking CTxMemPool::cs recursively in simple cases (hebasto)
- bitcoin/bitcoin#19478 Remove CTxMempool::mapLinks data structure member (JeremyRubin)
- bitcoin/bitcoin#19927 Reduce direct `g_chainman` usage (dongcarl)
- bitcoin/bitcoin#19898 log: print unexpected version warning in validation log category (n-thumann)
- bitcoin/bitcoin#20036 signet: Add assumed values for default signet (MarcoFalke)
- bitcoin/bitcoin#20048 chainparams: do not log signet startup messages for other chains (jonatack)
- bitcoin/bitcoin#19339 re-delegate absurd fee checking from mempool to clients (glozow)
- bitcoin/bitcoin#20035 signet: Fix uninitialized read in validation (MarcoFalke)
- bitcoin/bitcoin#20157 Bugfix: chainparams: Add missing (always enabled) Taproot deployment for Signet (luke-jr)
- bitcoin/bitcoin#20263 Update assumed chain params (MarcoFalke)
- bitcoin/bitcoin#20372 Avoid signed integer overflow when loading a mempool.dat file with a malformed time field (practicalswift)
- bitcoin/bitcoin#18621 script: Disallow silent bool -> cscript conversion (MarcoFalke)
- bitcoin/bitcoin#18612, bitcoin/bitcoin#18732 script: Remove undocumented and unused operator+ (MarcoFalke)
- bitcoin/bitcoin#19317 Add a left-justified width field to `log2_work` component for a uniform debug.log output (jamesgmorgan)

### P2P protocol and network code
- bitcoin/bitcoin#18544 Limit BIP37 filter lifespan (active between `filterload`..`filterclear`) (theStack)
- bitcoin/bitcoin#18806 Remove is{Empty,Full} flags from CBloomFilter, clarify CVE fix (theStack)
- bitcoin/bitcoin#18512 Improve asmap checks and add sanity check (sipa)
- bitcoin/bitcoin#18877 Serve cfcheckpt requests (jnewbery)
- bitcoin/bitcoin#18895 Unbroadcast followups: rpcs, nLastResend, mempool sanity check (gzhao408)
- bitcoin/bitcoin#19010 net processing: Add support for `getcfheaders` (jnewbery)
- bitcoin/bitcoin#16939 Delay querying DNS seeds (ajtowns)
- bitcoin/bitcoin#18807 Unbroadcast follow-ups (amitiuttarwar)
- bitcoin/bitcoin#19044 Add support for getcfilters (jnewbery)
- bitcoin/bitcoin#19084 improve code documentation for dns seed behaviour (ajtowns)
- bitcoin/bitcoin#19260 disconnect peers that send filterclear + update existing filter msg disconnect logic (gzhao408)
- bitcoin/bitcoin#19284 Add seed.bitcoin.wiz.biz to DNS seeds (wiz)
- bitcoin/bitcoin#19322 split PushInventory() (jnewbery)
- bitcoin/bitcoin#19204 Reduce inv traffic during IBD (MarcoFalke)
- bitcoin/bitcoin#19470 banlist: log post-swept banlist size at startup (fanquake)
- bitcoin/bitcoin#19191 Extract download permission from noban (MarcoFalke)
- bitcoin/bitcoin#14033 Drop `CADDR_TIME_VERSION` checks now that `MIN_PEER_PROTO_VERSION` is greater (Empact)
- bitcoin/bitcoin#19464 net, rpc: remove -banscore option, deprecate banscore in getpeerinfo (jonatack)
- bitcoin/bitcoin#19514 [net/net processing] check banman pointer before dereferencing (jnewbery)
- bitcoin/bitcoin#19512 banscore updates to gui, tests, release notes (jonatack)
- bitcoin/bitcoin#19360 improve encapsulation of CNetAddr (vasild)
- bitcoin/bitcoin#19217 disambiguate block-relay-only variable names from blocksonly variables (glowang)
- bitcoin/bitcoin#19473 Add -networkactive option (hebasto)
- bitcoin/bitcoin#19472 [net processing] Reduce `cs_main` scope in MaybeDiscourageAndDisconnect() (jnewbery)
- bitcoin/bitcoin#19583 clean up Misbehaving() (jnewbery)
- bitcoin/bitcoin#19534 save the network type explicitly in CNetAddr (vasild)
- bitcoin/bitcoin#19569 Enable fetching of orphan parents from wtxid peers (sipa)
- bitcoin/bitcoin#18991 Cache responses to GETADDR to prevent topology leaks (naumenkogs)
- bitcoin/bitcoin#19596 Deduplicate parent txid loop of requested transactions and missing parents of orphan transactions (sdaftuar)
- bitcoin/bitcoin#19316 Cleanup logic around connection types (amitiuttarwar)
- bitcoin/bitcoin#19070 Signal support for compact block filters with `NODE_COMPACT_FILTERS` (jnewbery)
- bitcoin/bitcoin#19705 Shrink CAddress from 48 to 40 bytes on x64 (vasild)
- bitcoin/bitcoin#19704 Move ProcessMessage() to PeerLogicValidation (jnewbery)
- bitcoin/bitcoin#19628 Change CNetAddr::ip to have flexible size (vasild)
- bitcoin/bitcoin#19797 Remove old check for 3-byte shifted IP addresses from pre-0.2.9 nodes (bitcoin/bitcoin#19797)
- bitcoin/bitcoin#19607 Add Peer struct for per-peer data in net processing (jnewbery)
- bitcoin/bitcoin#19857 improve nLastBlockTime and nLastTXTime documentation (jonatack)
- bitcoin/bitcoin#19724 Cleanup connection types- followups (amitiuttarwar)
- bitcoin/bitcoin#19670 Protect localhost and block-relay-only peers from eviction (sdaftuar)
- bitcoin/bitcoin#19728 Increase the ip address relay branching factor for unreachable networks (sipa)
- bitcoin/bitcoin#19879 Miscellaneous wtxid followups (amitiuttarwar)
- bitcoin/bitcoin#19697 Improvements on ADDR caching (naumenkogs)
- bitcoin/bitcoin#17785 Unify Send and Receive protocol versions (hebasto)
- bitcoin/bitcoin#19845 CNetAddr: add support to (un)serialize as ADDRv2 (vasild)
- bitcoin/bitcoin#19107 Move all header verification into the network layer, extend logging (troygiorshev)
- bitcoin/bitcoin#20003 Exit with error message if -proxy is specified without arguments (instead of continuing without proxy server) (practicalswift)
- bitcoin/bitcoin#19991 Use alternative port for incoming Tor connections (hebasto)
- bitcoin/bitcoin#19723 Ignore unknown messages before VERACK (sdaftuar)
- bitcoin/bitcoin#19954 Complete the BIP155 implementation and upgrade to TORv3 (vasild)
- bitcoin/bitcoin#20119 BIP155 follow-ups (sipa)
- bitcoin/bitcoin#19988 Overhaul transaction request logic (sipa)
- bitcoin/bitcoin#17428 Try to preserve outbound block-relay-only connections during restart (hebasto)
- bitcoin/bitcoin#19911 Guard `vRecvGetData` with `cs_vRecv` and `orphan_work_set` with `g_cs_orphans` (narula)
- bitcoin/bitcoin#19753 Don't add AlreadyHave transactions to recentRejects (troygiorshev)
- bitcoin/bitcoin#20187 Test-before-evict bugfix and improvements for block-relay-only peers (sdaftuar)
- bitcoin/bitcoin#20237 Hardcoded seeds update for 0.21 (laanwj)
- bitcoin/bitcoin#20212 Fix output of peer address in version message (vasild)
- bitcoin/bitcoin#20284 Ensure old versions don't parse peers.dat (vasild)
- bitcoin/bitcoin#20405 Avoid calculating onion address checksum when version is not 3 (lontivero)
- bitcoin/bitcoin#20564 Don't send 'sendaddrv2' to pre-70016 software, and send before 'verack' (sipa)
- bitcoin/bitcoin#20660 Move signet onion seed from v2 to v3 (Sjors)
- bitcoin/bitcoin#20852 allow CSubNet of non-IP networks (vasild)
- bitcoin/bitcoin#21043 Avoid UBSan warning in ProcessMessage(…) (practicalswift)
- bitcoin/bitcoin#21644 use NetPermissions::HasFlag() in CConnman::Bind() (jonatack)
- bitcoin/bitcoin#22569 Rate limit the processing of rumoured addresses (sipa)

### Wallet
- bitcoin/bitcoin#18262 Exit selection when `best_waste` is 0 (achow101)
- bitcoin/bitcoin#17824 Prefer full destination groups in coin selection (fjahr)
- bitcoin/bitcoin#17219 Allow transaction without change if keypool is empty (Sjors)
- bitcoin/bitcoin#15761 Replace -upgradewallet startup option with upgradewallet RPC (achow101)
- bitcoin/bitcoin#18671 Add BlockUntilSyncedToCurrentChain to dumpwallet (MarcoFalke)
- bitcoin/bitcoin#16528 Native Descriptor Wallets using DescriptorScriptPubKeyMan (achow101)
- bitcoin/bitcoin#18777 Recommend absolute path for dumpwallet (MarcoFalke)
- bitcoin/bitcoin#16426 Reverse `cs_main`, `cs_wallet` lock order and reduce `cs_main` locking (ariard)
- bitcoin/bitcoin#18699 Avoid translating RPC errors (MarcoFalke)
- bitcoin/bitcoin#18782 Make sure no DescriptorScriptPubKeyMan or WalletDescriptor members are left uninitialized after construction (practicalswift)
- bitcoin/bitcoin#9381 Remove CWalletTx merging logic from AddToWallet (ryanofsky)
- bitcoin/bitcoin#16946 Include a checksum of encrypted private keys (achow101)
- bitcoin/bitcoin#17681 Keep inactive seeds after sethdseed and derive keys from them as needed (achow101)
- bitcoin/bitcoin#18918 Move salvagewallet into wallettool (achow101)
- bitcoin/bitcoin#14988 Fix for confirmed column in csv export for payment to self transactions (benthecarman)
- bitcoin/bitcoin#18275 Error if an explicit fee rate was given but the needed fee rate differed (kallewoof)
- bitcoin/bitcoin#19054 Skip hdKeypath of 'm' when determining inactive hd seeds (achow101)
- bitcoin/bitcoin#17938 Disallow automatic conversion between disparate hash types (Empact)
- bitcoin/bitcoin#19237 Check size after unserializing a pubkey (elichai)
- bitcoin/bitcoin#11413 sendtoaddress/sendmany: Add explicit feerate option (kallewoof)
- bitcoin/bitcoin#18850 Fix ZapSelectTx to sync wallet spends (bvbfan)
- bitcoin/bitcoin#18923 Never schedule MaybeCompactWalletDB when `-flushwallet` is off (MarcoFalke)
- bitcoin/bitcoin#19441 walletdb: Don't reinitialize desc cache with multiple cache entries (achow101)
- bitcoin/bitcoin#18907 walletdb: Don't remove database transaction logs and instead error (achow101)
- bitcoin/bitcoin#19334 Introduce WalletDatabase abstract class (achow101)
- bitcoin/bitcoin#19335 Cleanup and separate BerkeleyDatabase and BerkeleyBatch (achow101)
- bitcoin/bitcoin#19102 Introduce and use DummyDatabase instead of dummy BerkeleyDatabase (achow101)
- bitcoin/bitcoin#19568 Wallet should not override signing errors (fjahr)
- bitcoin/bitcoin#17204 Do not turn `OP_1NEGATE` in scriptSig into `0x0181` in signing code (sipa) (meshcollider)
- bitcoin/bitcoin#19457 Cleanup wallettool salvage and walletdb extraneous declarations (achow101)
- bitcoin/bitcoin#15937 Add loadwallet and createwallet `load_on_startup` options (ryanofsky)
- bitcoin/bitcoin#16841 Replace GetScriptForWitness with GetScriptForDestination (meshcollider)
- bitcoin/bitcoin#14582 always do avoid partial spends if fees are within a specified range (kallewoof)
- bitcoin/bitcoin#19743 -maxapsfee follow-up (kallewoof)
- bitcoin/bitcoin#19289 GetWalletTx and IsMine require `cs_wallet` lock (promag)
- bitcoin/bitcoin#19671 Remove -zapwallettxes (achow101)
- bitcoin/bitcoin#19805 Avoid deserializing unused records when salvaging (achow101)
- bitcoin/bitcoin#19754 wallet, gui: Reload previously loaded wallets on startup (achow101)
- bitcoin/bitcoin#19738 Avoid multiple BerkeleyBatch in DelAddressBook (promag)
- bitcoin/bitcoin#19919 bugfix: make LoadWallet assigns status always (AkioNak)
- bitcoin/bitcoin#16378 The ultimate send RPC (Sjors)
- bitcoin/bitcoin#15454 Remove the automatic creation and loading of the default wallet (achow101)
- bitcoin/bitcoin#19501 `send*` RPCs in the wallet returns the "fee reason" (stackman27)
- bitcoin/bitcoin#20130 Remove db mode string (S3RK)
- bitcoin/bitcoin#19077 Add sqlite as an alternative wallet database and use it for new descriptor wallets (achow101)
- bitcoin/bitcoin#20125 Expose database format in getwalletinfo (promag)
- bitcoin/bitcoin#20198 Show name, format and if uses descriptors in bitcoin-wallet tool (jonasschnelli)
- bitcoin/bitcoin#20216 Fix buffer over-read in SQLite file magic check (theStack)
- bitcoin/bitcoin#20186 Make -wallet setting not create wallets (ryanofsky)
- bitcoin/bitcoin#20230 Fix bug when just created encrypted wallet cannot get address (hebasto)
- bitcoin/bitcoin#20282 Change `upgradewallet` return type to be an object (jnewbery)
- bitcoin/bitcoin#20220 Explicit fee rate follow-ups/fixes for 0.21 (jonatack)
- bitcoin/bitcoin#20199 Ignore (but warn) on duplicate -wallet parameters (jonasschnelli)
- bitcoin/bitcoin#20324 Set DatabaseStatus::SUCCESS in MakeSQLiteDatabase (MarcoFalke)
- bitcoin/bitcoin#20266 Fix change detection of imported internal descriptors (achow101)
- bitcoin/bitcoin#20153 Do not import a descriptor with hardened derivations into a watch-only wallet (S3RK)
- bitcoin/bitcoin#20344 Fix scanning progress calculation for single block range (theStack)
- bitcoin/bitcoin#19502 Bugfix: Wallet: Soft-fail exceptions within ListWalletDir file checks (luke-jr)
- bitcoin/bitcoin#20378 Fix potential division by 0 in WalletLogPrintf (jonasschnelli)
- bitcoin/bitcoin#18836 Upgradewallet fixes and additional tests (achow101)
- bitcoin/bitcoin#20139 Do not return warnings from UpgradeWallet() (stackman27)
- bitcoin/bitcoin#20305 Introduce `fee_rate` sat/vB param/option (jonatack)
- bitcoin/bitcoin#20426 Allow zero-fee fundrawtransaction/walletcreatefundedpsbt and other fixes (jonatack)
- bitcoin/bitcoin#20573 wallet, bugfix: allow send with string `fee_rate` amounts (jonatack)
- bitcoin/bitcoin#21166 Introduce DeferredSignatureChecker and have SignatureExtractorClass subclass it (achow101)
- bitcoin/bitcoin#21083 Avoid requesting fee rates multiple times during coin selection (achow101)
- bitcoin/bitcoin#21907 Do not iterate a directory if having an error while accessing it (hebasto)

### RPC and other APIs
- bitcoin/bitcoin#18574 cli: Call getbalances.ismine.trusted instead of getwalletinfo.balance (jonatack)
- bitcoin/bitcoin#17693 Add `generateblock` to mine a custom set of transactions (andrewtoth)
- bitcoin/bitcoin#18495 Remove deprecated migration code (vasild)
- bitcoin/bitcoin#18493 Remove deprecated "size" from mempool txs (vasild)
- bitcoin/bitcoin#18467 Improve documentation and return value of settxfee (fjahr)
- bitcoin/bitcoin#18607 Fix named arguments in documentation (MarcoFalke)
- bitcoin/bitcoin#17831 doc: Fix and extend getblockstats examples (asoltys)
- bitcoin/bitcoin#18785 Prevent valgrind false positive in `rest_blockhash_by_height` (ryanofsky)
- bitcoin/bitcoin#18999 log: Remove "No rpcpassword set" from logs (MarcoFalke)
- bitcoin/bitcoin#19006 Avoid crash when `g_thread_http` was never started (MarcoFalke)
- bitcoin/bitcoin#18594 cli: Display multiwallet balances in -getinfo (jonatack)
- bitcoin/bitcoin#19056 Make gettxoutsetinfo/GetUTXOStats interruptible (MarcoFalke)
- bitcoin/bitcoin#19112 Remove special case for unknown service flags (MarcoFalke)
- bitcoin/bitcoin#18826 Expose txinwitness for coinbase in JSON form from RPC (rvagg)
- bitcoin/bitcoin#19282 Rephrase generatetoaddress help, and use `PACKAGE_NAME` (luke-jr)
- bitcoin/bitcoin#16377 don't automatically append inputs in walletcreatefundedpsbt (Sjors)
- bitcoin/bitcoin#19200 Remove deprecated getaddressinfo fields (jonatack)
- bitcoin/bitcoin#19133 rpc, cli, test: add bitcoin-cli -generate command (jonatack)
- bitcoin/bitcoin#19469 Deprecate banscore field in getpeerinfo (jonatack)
- bitcoin/bitcoin#16525 Dump transaction version as an unsigned integer in RPC/TxToUniv (TheBlueMatt)
- bitcoin/bitcoin#19555 Deduplicate WriteHDKeypath() used in decodepsbt (theStack)
- bitcoin/bitcoin#19589 Avoid useless mempool query in gettxoutproof (MarcoFalke)
- bitcoin/bitcoin#19585 RPCResult Type of MempoolEntryDescription should be OBJ (stylesuxx)
- bitcoin/bitcoin#19634 Document getwalletinfo's `unlocked_until` field as optional (justinmoon)
- bitcoin/bitcoin#19658 Allow RPC to fetch all addrman records and add records to addrman (jnewbery)
- bitcoin/bitcoin#19696 Fix addnode remove command error (fjahr)
- bitcoin/bitcoin#18654 Separate bumpfee's psbt creation function into psbtbumpfee (achow101)
- bitcoin/bitcoin#19655 Catch listsinceblock `target_confirmations` exceeding block count (adaminsky)
- bitcoin/bitcoin#19644 Document returned error fields as optional if applicable (theStack)
- bitcoin/bitcoin#19455 rpc generate: print useful help and error message (jonatack)
- bitcoin/bitcoin#19550 Add listindices RPC (fjahr)
- bitcoin/bitcoin#19169 Validate provided keys for `query_options` parameter in listunspent (PastaPastaPasta)
- bitcoin/bitcoin#18244 fundrawtransaction and walletcreatefundedpsbt also lock manually selected coins (Sjors)
- bitcoin/bitcoin#14687 zmq: Enable TCP keepalive (mruddy)
- bitcoin/bitcoin#19405 Add network in/out connections to `getnetworkinfo` and `-getinfo` (jonatack)
- bitcoin/bitcoin#19878 rawtransaction: Fix argument in combinerawtransaction help message (pinheadmz)
- bitcoin/bitcoin#19940 Return fee and vsize from testmempoolaccept (gzhao408)
- bitcoin/bitcoin#13686 zmq: Small cleanups in the ZMQ code (domob1812)
- bitcoin/bitcoin#19386, bitcoin/bitcoin#19528, bitcoin/bitcoin#19717, bitcoin/bitcoin#19849, bitcoin/bitcoin#19994 Assert that RPCArg names are equal to CRPCCommand ones (MarcoFalke)
- bitcoin/bitcoin#19725 Add connection type to getpeerinfo, improve logs (amitiuttarwar)
- bitcoin/bitcoin#19969 Send RPC bug fix and touch-ups (Sjors)
- bitcoin/bitcoin#18309 zmq: Add support to listen on multiple interfaces (n-thumann)
- bitcoin/bitcoin#20055 Set HTTP Content-Type in bitcoin-cli (laanwj)
- bitcoin/bitcoin#19956 Improve invalid vout value rpc error message (n1rna)
- bitcoin/bitcoin#20101 Change no wallet loaded message to be clearer (achow101)
- bitcoin/bitcoin#19998 Add `via_tor` to `getpeerinfo` output (hebasto)
- bitcoin/bitcoin#19770 getpeerinfo: Deprecate "whitelisted" field (replaced by "permissions") (luke-jr)
- bitcoin/bitcoin#20120 net, rpc, test, bugfix: update GetNetworkName, GetNetworksInfo, regression tests (jonatack)
- bitcoin/bitcoin#20595 Improve heuristic hex transaction decoding (sipa)
- bitcoin/bitcoin#20731 Add missing description of vout in getrawtransaction help text (benthecarman)
- bitcoin/bitcoin#19328 Add gettxoutsetinfo `hash_type` option (fjahr)
- bitcoin/bitcoin#19731 Expose nLastBlockTime/nLastTXTime as last `block/last_transaction` in getpeerinfo (jonatack)
- bitcoin/bitcoin#19572 zmq: Create "sequence" notifier, enabling client-side mempool tracking (instagibbs)
- bitcoin/bitcoin#20002 Expose peer network in getpeerinfo; simplify/improve -netinfo (jonatack)
- bitcoin/bitcoin#21201 Disallow sendtoaddress and sendmany when private keys disabled (achow101)
- bitcoin/bitcoin#19361 Reset scantxoutset progress before inferring descriptors (prusnak)

### GUI
- bitcoin/bitcoin#17905 Avoid redundant tx status updates (ryanofsky)
- bitcoin/bitcoin#18646 Use `PACKAGE_NAME` in exception message (fanquake)
- bitcoin/bitcoin#17509 Save and load PSBT (Sjors)
- bitcoin/bitcoin#18769 Remove bug fix for Qt < 5.5 (10xcryptodev)
- bitcoin/bitcoin#15768 Add close window shortcut (IPGlider)
- bitcoin/bitcoin#16224 Bilingual GUI error messages (hebasto)
- bitcoin/bitcoin#18922 Do not translate InitWarning messages in debug.log (hebasto)
- bitcoin/bitcoin#18152 Use NotificationStatus enum for signals to GUI (hebasto)
- bitcoin/bitcoin#18587 Avoid wallet tryGetBalances calls in WalletModel::pollBalanceChanged (ryanofsky)
- bitcoin/bitcoin#17597 Fix height of QR-less ReceiveRequestDialog (hebasto)
- bitcoin/bitcoin#17918 Hide non PKHash-Addresses in signing address book (emilengler)
- bitcoin/bitcoin#17956 Disable unavailable context menu items in transactions tab (kristapsk)
- bitcoin/bitcoin#17968 Ensure that ModalOverlay is resized properly (hebasto)
- bitcoin/bitcoin#17993 Balance/TxStatus polling update based on last block hash (furszy)
- bitcoin/bitcoin#18424 Use parent-child relation to manage lifetime of OptionsModel object (hebasto)
- bitcoin/bitcoin#18452 Fix shutdown when `waitfor*` cmds are called from RPC console (hebasto)
- bitcoin/bitcoin#15202 Add Close All Wallets action (promag)
- bitcoin/bitcoin#19132 lock `cs_main`, `m_cached_tip_mutex` in that order (vasild)
- bitcoin/bitcoin#18898 Display warnings as rich text (hebasto)
- bitcoin/bitcoin#19231 add missing translation.h include to fix build (fanquake)
- bitcoin/bitcoin#18027 "PSBT Operations" dialog (gwillen)
- bitcoin/bitcoin#19256 Change combiner for signals to `optional_last_value` (fanquake)
- bitcoin/bitcoin#18896 Reset toolbar after all wallets are closed (hebasto)
- bitcoin/bitcoin#18993 increase console command max length (10xcryptodev)
- bitcoin/bitcoin#19323 Fix regression in *txoutset* in GUI console (hebasto)
- bitcoin/bitcoin#19210 Get rid of cursor in out-of-focus labels (hebasto)
- bitcoin/bitcoin#19011 Reduce `cs_main` lock accumulation during GUI startup (jonasschnelli)
- bitcoin/bitcoin#19844 Remove usage of boost::bind (fanquake)
- bitcoin/bitcoin#20479 Fix QPainter non-determinism on macOS (0.21 backport) (laanwj)
- bitcoin-core/gui#6 Do not truncate node flag strings in debugwindow peers details tab (Saibato)
- bitcoin-core/gui#8 Fix regression in TransactionTableModel (hebasto)
- bitcoin-core/gui#17 doc: Remove outdated comment in TransactionTablePriv (MarcoFalke)
- bitcoin-core/gui#20 Wrap tooltips in the intro window (hebasto)
- bitcoin-core/gui#30 Disable the main window toolbar when the modal overlay is shown (hebasto)
- bitcoin-core/gui#34 Show permissions instead of whitelisted (laanwj)
- bitcoin-core/gui#35 Parse params directly instead of through node (ryanofsky)
- bitcoin-core/gui#39 Add visual accenting for the 'Create new receiving address' button (hebasto)
- bitcoin-core/gui#40 Clarify block height label (hebasto)
- bitcoin-core/gui#43 bugfix: Call setWalletActionsEnabled(true) only for the first wallet (hebasto)
- bitcoin-core/gui#97 Relax GUI freezes during IBD (jonasschnelli)
- bitcoin-core/gui#71 Fix visual quality of text in QR image (hebasto)
- bitcoin-core/gui#96 Slight improve create wallet dialog (Sjors)
- bitcoin-core/gui#102 Fix SplashScreen crash when run with -disablewallet (hebasto)
- bitcoin-core/gui#116 Fix unreasonable default size of the main window without loaded wallets (hebasto)
- bitcoin-core/gui#120 Fix multiwallet transaction notifications (promag)
- bitcoin/bitcoin#277 Do not use QClipboard::Selection on Windows and macOS. (hebasto)
- bitcoin/bitcoin#280 Remove user input from URI error message (prayank23)
- bitcoin/bitcoin#365 Draw "eye" sign at the beginning of watch-only addresses (hebasto)

### Build system
- bitcoin/bitcoin#18504 Drop bitcoin-tx and bitcoin-wallet dependencies on libevent (ryanofsky)
- bitcoin/bitcoin#18586 Bump gitian descriptors to 0.21 (laanwj)
- bitcoin/bitcoin#17595 guix: Enable building for `x86_64-w64-mingw32` target (dongcarl)
- bitcoin/bitcoin#17929 add linker optimisation flags to gitian & guix (Linux) (fanquake)
- bitcoin/bitcoin#18556 Drop make dist in gitian builds (hebasto)
- bitcoin/bitcoin#18088 ensure we aren't using GNU extensions (fanquake)
- bitcoin/bitcoin#18741 guix: Make source tarball using git-archive (dongcarl)
- bitcoin/bitcoin#18843 warn on potentially uninitialized reads (vasild)
- bitcoin/bitcoin#17874 make linker checks more robust (fanquake)
- bitcoin/bitcoin#18535 remove -Qunused-arguments workaround for clang + ccache (fanquake)
- bitcoin/bitcoin#18743 Add --sysroot option to mac os native compile flags (ryanofsky)
- bitcoin/bitcoin#18216 test, build: Enable -Werror=sign-compare (Empact)
- bitcoin/bitcoin#18928 don't pass -w when building for Windows (fanquake)
- bitcoin/bitcoin#16710 Enable -Wsuggest-override if available (hebasto)
- bitcoin/bitcoin#18738 Suppress -Wdeprecated-copy warnings (hebasto)
- bitcoin/bitcoin#18862 Remove fdelt_chk back-compat code and sanity check (fanquake)
- bitcoin/bitcoin#18887 enable -Werror=gnu (vasild)
- bitcoin/bitcoin#18956 enforce minimum required Windows version (7) (fanquake)
- bitcoin/bitcoin#18958 guix: Make V=1 more powerful for debugging (dongcarl)
- bitcoin/bitcoin#18677 Multiprocess build support (ryanofsky)
- bitcoin/bitcoin#19094 Only allow ASCII identifiers (laanwj)
- bitcoin/bitcoin#18820 Propagate well-known vars into depends (dongcarl)
- bitcoin/bitcoin#19173 turn on --enable-c++17 by --enable-fuzz (vasild)
- bitcoin/bitcoin#18297 Use pkg-config in BITCOIN_QT_CONFIGURE for all hosts including Windows (hebasto)
- bitcoin/bitcoin#19301 don't warn when doxygen isn't found (fanquake)
- bitcoin/bitcoin#19240 macOS toolchain simplification and bump (dongcarl)
- bitcoin/bitcoin#19356 Fix search for brew-installed BDB 4 on OS X (gwillen)
- bitcoin/bitcoin#19394 Remove unused `RES_IMAGES` (Bushstar)
- bitcoin/bitcoin#19403 improve `__builtin_clz*` detection (fanquake)
- bitcoin/bitcoin#19375 target Windows 7 when building libevent and fix ipv6 usage (fanquake)
- bitcoin/bitcoin#19331 Do not include server symbols in wallet (MarcoFalke)
- bitcoin/bitcoin#19257 remove BIP70 configure option (fanquake)
- bitcoin/bitcoin#18288 Add MemorySanitizer (MSan) in Travis to detect use of uninitialized memory (practicalswift)
- bitcoin/bitcoin#18307 Require pkg-config for all of the hosts (hebasto)
- bitcoin/bitcoin#19445 Update msvc build to use ISO standard C++17 (sipsorcery)
- bitcoin/bitcoin#18882 fix -Wformat-security check when compiling with GCC (fanquake)
- bitcoin/bitcoin#17919 Allow building with system clang (dongcarl)
- bitcoin/bitcoin#19553 pass -fcommon when building genisoimage (fanquake)
- bitcoin/bitcoin#19565 call `AC_PATH_TOOL` for dsymutil in macOS cross-compile (fanquake)
- bitcoin/bitcoin#19530 build LTO support into Apple's ld64 (theuni)
- bitcoin/bitcoin#19525 add -Wl,-z,separate-code to hardening flags (fanquake)
- bitcoin/bitcoin#19667 set minimum required Boost to 1.58.0 (fanquake)
- bitcoin/bitcoin#19672 make clean removes .gcda and .gcno files from fuzz directory (Crypt-iQ)
- bitcoin/bitcoin#19622 Drop ancient hack in gitian-linux descriptor (hebasto)
- bitcoin/bitcoin#19688 Add support for llvm-cov (hebasto)
- bitcoin/bitcoin#19718 Add missed gcov files to 'make clean' (hebasto)
- bitcoin/bitcoin#19719 Add Werror=range-loop-analysis (MarcoFalke)
- bitcoin/bitcoin#19015 Enable some commonly enabled compiler diagnostics (practicalswift)
- bitcoin/bitcoin#19689 build, qt: Add Qt version checking (hebasto)
- bitcoin/bitcoin#17396 modest Android improvements (icota)
- bitcoin/bitcoin#18405 Drop all of the ZeroMQ patches (hebasto)
- bitcoin/bitcoin#15704 Move Win32 defines to configure.ac to ensure they are globally defined (luke-jr)
- bitcoin/bitcoin#19761 improve sed robustness by not using sed (fanquake)
- bitcoin/bitcoin#19758 Drop deprecated and unused `GUARDED_VAR` and `PT_GUARDED_VAR` annotations (hebasto)
- bitcoin/bitcoin#18921 add stack-clash and control-flow protection options to hardening flags (fanquake)
- bitcoin/bitcoin#19803 Bugfix: Define and use `HAVE_FDATASYNC` correctly outside LevelDB (luke-jr)
- bitcoin/bitcoin#19685 CMake invocation cleanup (dongcarl)
- bitcoin/bitcoin#19861 add /usr/local/ to `LCOV_FILTER_PATTERN` for macOS builds (Crypt-iQ)
- bitcoin/bitcoin#19916 allow user to specify `DIR_FUZZ_SEED_CORPUS` for `cov_fuzz` (Crypt-iQ)
- bitcoin/bitcoin#19944 Update secp256k1 subtree (including BIP340 support) (sipa)
- bitcoin/bitcoin#19558 Split pthread flags out of ldflags and dont use when building libconsensus (fanquake)
- bitcoin/bitcoin#19959 patch qt libpng to fix powerpc build (fanquake)
- bitcoin/bitcoin#19868 Fix target name (hebasto)
- bitcoin/bitcoin#19960 The vcpkg tool has introduced a proper way to use manifests (sipsorcery)
- bitcoin/bitcoin#20065 fuzz: Configure check for main function (MarcoFalke)
- bitcoin/bitcoin#18750 Optionally skip external warnings (vasild)
- bitcoin/bitcoin#20147 Update libsecp256k1 (endomorphism, test improvements) (sipa)
- bitcoin/bitcoin#20156 Make sqlite support optional (compile-time) (luke-jr)
- bitcoin/bitcoin#20318 Ensure source tarball has leading directory name (MarcoFalke)
- bitcoin/bitcoin#20447 Patch `qt_intersect_spans` to avoid non-deterministic behavior in LLVM 8 (achow101)
- bitcoin/bitcoin#20505 Avoid secp256k1.h include from system (dergoegge)
- bitcoin/bitcoin#20527 Do not ignore Homebrew's SQLite on macOS (hebasto)
- bitcoin/bitcoin#20478 Don't set BDB flags when configuring without (jonasschnelli)
- bitcoin/bitcoin#20563 Check that Homebrew's berkeley-db4 package is actually installed (hebasto)
- bitcoin/bitcoin#19493 Fix clang build on Mac (bvbfan)
- bitcoin/bitcoin#21486 link against -lsocket if required for `*ifaddrs` (fanquake)
- bitcoin/bitcoin#20983 Fix MSVC build after bitcoin-core/gui#176 (hebasto)
- bitcoin/bitcoin#21932 depends: update Qt 5.9 source url (kittywhiskers)
- bitcoin/bitcoin#22017 Update Windows code signing certificate (achow101)
- bitcoin/bitcoin#22191 Use custom MacOS code signing tool (achow101)
- bitcoin/bitcoin#22713 Fix build with Boost 1.77.0 (sizeofvoid)

### Tests and QA
- bitcoin/bitcoin#18593 Complete impl. of `msg_merkleblock` and `wait_for_merkleblock` (theStack)
- bitcoin/bitcoin#18609 Remove REJECT message code (hebasto)
- bitcoin/bitcoin#18584 Check that the version message does not leak the local address (MarcoFalke)
- bitcoin/bitcoin#18597 Extend `wallet_dump` test to cover comments (MarcoFalke)
- bitcoin/bitcoin#18596 Try once more when RPC connection fails on Windows (MarcoFalke)
- bitcoin/bitcoin#18451 shift coverage from getunconfirmedbalance to getbalances (jonatack)
- bitcoin/bitcoin#18631 appveyor: Disable functional tests for now (MarcoFalke)
- bitcoin/bitcoin#18628 Add various low-level p2p tests (MarcoFalke)
- bitcoin/bitcoin#18615 Avoid accessing free'd memory in `validation_chainstatemanager_tests` (MarcoFalke)
- bitcoin/bitcoin#18571 fuzz: Disable debug log file (MarcoFalke)
- bitcoin/bitcoin#18653 add coverage for bitcoin-cli -rpcwait (jonatack)
- bitcoin/bitcoin#18660 Verify findCommonAncestor always initializes outputs (ryanofsky)
- bitcoin/bitcoin#17669 Have coins simulation test also use CCoinsViewDB (jamesob)
- bitcoin/bitcoin#18662 Replace gArgs with local argsman in bench (MarcoFalke)
- bitcoin/bitcoin#18641 Create cached blocks not in the future (MarcoFalke)
- bitcoin/bitcoin#18682 fuzz: `http_request` workaround for libevent < 2.1.1 (theStack)
- bitcoin/bitcoin#18692 Bump timeout in `wallet_import_rescan` (MarcoFalke)
- bitcoin/bitcoin#18695 Replace boost::mutex with std::mutex (hebasto)
- bitcoin/bitcoin#18633 Properly raise FailedToStartError when rpc shutdown before warmup finished (MarcoFalke)
- bitcoin/bitcoin#18675 Don't initialize PrecomputedTransactionData in txvalidationcache tests (jnewbery)
- bitcoin/bitcoin#18691 Add `wait_for_cookie_credentials()` to framework for rpcwait tests (jonatack)
- bitcoin/bitcoin#18672 Add further BIP37 size limit checks to `p2p_filter.py` (theStack)
- bitcoin/bitcoin#18721 Fix linter issue (hebasto)
- bitcoin/bitcoin#18384 More specific `feature_segwit` test error messages and fixing incorrect comments (gzhao408)
- bitcoin/bitcoin#18575 bench: Remove requirement that all benches use same testing setup (MarcoFalke)
- bitcoin/bitcoin#18690 Check object hashes in `wait_for_getdata` (robot-visions)
- bitcoin/bitcoin#18712 display command line options passed to `send_cli()` in debug log (jonatack)
- bitcoin/bitcoin#18745 Check submitblock return values (MarcoFalke)
- bitcoin/bitcoin#18756 Use `wait_for_getdata()` in `p2p_compactblocks.py` (theStack)
- bitcoin/bitcoin#18724 Add coverage for -rpcwallet cli option (jonatack)
- bitcoin/bitcoin#18754 bench: Add caddrman benchmarks (vasild)
- bitcoin/bitcoin#18585 Use zero-argument super() shortcut (Python 3.0+) (theStack)
- bitcoin/bitcoin#18688 fuzz: Run in parallel (MarcoFalke)
- bitcoin/bitcoin#18770 Remove raw-tx byte juggling in `mempool_reorg` (MarcoFalke)
- bitcoin/bitcoin#18805 Add missing `sync_all` to `wallet_importdescriptors.py` (achow101)
- bitcoin/bitcoin#18759 bench: Start nodes with -nodebuglogfile (MarcoFalke)
- bitcoin/bitcoin#18774 Added test for upgradewallet RPC (brakmic)
- bitcoin/bitcoin#18485 Add `mempool_updatefromblock.py` (hebasto)
- bitcoin/bitcoin#18727 Add CreateWalletFromFile test (ryanofsky)
- bitcoin/bitcoin#18726 Check misbehavior more independently in `p2p_filter.py` (robot-visions)
- bitcoin/bitcoin#18825 Fix message for `ECC_InitSanityCheck` test (fanquake)
- bitcoin/bitcoin#18576 Use unittest for `test_framework` unit testing (gzhao408)
- bitcoin/bitcoin#18828 Strip down previous releases boilerplate (MarcoFalke)
- bitcoin/bitcoin#18617 Add factor option to adjust test timeouts (brakmic)
- bitcoin/bitcoin#18855 `feature_backwards_compatibility.py` test downgrade after upgrade (achow101)
- bitcoin/bitcoin#18864 Add v0.16.3 backwards compatibility test, bump v0.19.0.1 to v0.19.1 (Sjors)
- bitcoin/bitcoin#18917 fuzz: Fix vector size problem in system fuzzer (brakmic)
- bitcoin/bitcoin#18901 fuzz: use std::optional for `sep_pos_opt` variable (brakmic)
- bitcoin/bitcoin#18888 Remove RPCOverloadWrapper boilerplate (MarcoFalke)
- bitcoin/bitcoin#18952 Avoid os-dependent path (fametrano)
- bitcoin/bitcoin#18938 Fill fuzzing coverage gaps for functions in consensus/validation.h, primitives/block.h and util/translation.h (practicalswift)
- bitcoin/bitcoin#18986 Add capability to disable RPC timeout in functional tests (rajarshimaitra)
- bitcoin/bitcoin#18530 Add test for -blocksonly and -whitelistforcerelay param interaction (glowang)
- bitcoin/bitcoin#19014 Replace `TEST_PREVIOUS_RELEASES` env var with `test_framework` option (MarcoFalke)
- bitcoin/bitcoin#19052 Don't limit fuzzing inputs to 1 MB for afl-fuzz (now: ∞ ∀ fuzzers) (practicalswift)
- bitcoin/bitcoin#19060 Remove global `wait_until` from `p2p_getdata` (MarcoFalke)
- bitcoin/bitcoin#18926 Pass ArgsManager into `getarg_tests` (glowang)
- bitcoin/bitcoin#19110 Explain that a bug should be filed when the tests fail (MarcoFalke)
- bitcoin/bitcoin#18965 Implement `base58_decode` (10xcryptodev)
- bitcoin/bitcoin#16564 Always define the `raii_event_tests` test suite (candrews)
- bitcoin/bitcoin#19122 Add missing `sync_blocks` to `wallet_hd` (MarcoFalke)
- bitcoin/bitcoin#18875 fuzz: Stop nodes in `process_message*` fuzzers (MarcoFalke)
- bitcoin/bitcoin#18974 Check that invalid witness destinations can not be imported (MarcoFalke)
- bitcoin/bitcoin#18210 Type hints in Python tests (kiminuo)
- bitcoin/bitcoin#19159 Make valgrind.supp work on aarch64 (MarcoFalke)
- bitcoin/bitcoin#19082 Moved the CScriptNum asserts into the unit test in script.py (gillichu)
- bitcoin/bitcoin#19172 Do not swallow flake8 exit code (hebasto)
- bitcoin/bitcoin#19188 Avoid overwriting the NodeContext member of the testing setup [-Wshadow-field] (MarcoFalke)
- bitcoin/bitcoin#18890 `disconnect_nodes` should warn if nodes were already disconnected (robot-visions)
- bitcoin/bitcoin#19227 change blacklist to blocklist (TrentZ)
- bitcoin/bitcoin#19230 Move base58 to own module to break circular dependency (sipa)
- bitcoin/bitcoin#19083 `msg_mempool`, `fRelay`, and other bloomfilter tests (gzhao408)
- bitcoin/bitcoin#16756 Connection eviction logic tests (mzumsande)
- bitcoin/bitcoin#19177 Fix and clean `p2p_invalid_messages` functional tests (troygiorshev)
- bitcoin/bitcoin#19264 Don't import asyncio to test magic bytes (jnewbery)
- bitcoin/bitcoin#19178 Make `mininode_lock` non-reentrant (jnewbery)
- bitcoin/bitcoin#19153 Mempool compatibility test (S3RK)
- bitcoin/bitcoin#18434 Add a test-security target and run it in CI (fanquake)
- bitcoin/bitcoin#19252 Wait for disconnect in `disconnect_p2ps` + bloomfilter test followups (gzhao408)
- bitcoin/bitcoin#19298 Add missing `sync_blocks` (MarcoFalke)
- bitcoin/bitcoin#19304 Check that message sends successfully when header is split across two buffers (troygiorshev)
- bitcoin/bitcoin#19208 move `sync_blocks` and `sync_mempool` functions to `test_framework.py` (ycshao)
- bitcoin/bitcoin#19198 Check that peers with forcerelay permission are not asked to feefilter (MarcoFalke)
- bitcoin/bitcoin#19351 add two edge case tests for CSubNet (vasild)
- bitcoin/bitcoin#19272 net, test: invalid p2p messages and test framework improvements (jonatack)
- bitcoin/bitcoin#19348 Bump linter versions (duncandean)
- bitcoin/bitcoin#19366 Provide main(…) function in fuzzer. Allow building uninstrumented harnesses with --enable-fuzz (practicalswift)
- bitcoin/bitcoin#19412 move `TEST_RUNNER_EXTRA` into native tsan setup (fanquake)
- bitcoin/bitcoin#19368 Improve functional tests compatibility with BSD/macOS (S3RK)
- bitcoin/bitcoin#19028 Set -logthreadnames in unit tests (MarcoFalke)
- bitcoin/bitcoin#18649 Add std::locale::global to list of locale dependent functions (practicalswift)
- bitcoin/bitcoin#19140 Avoid fuzzer-specific nullptr dereference in libevent when handling PROXY requests (practicalswift)
- bitcoin/bitcoin#19214 Auto-detect SHA256 implementation in benchmarks (sipa)
- bitcoin/bitcoin#19353 Fix mistakenly swapped "previous" and "current" lock orders (hebasto)
- bitcoin/bitcoin#19533 Remove unnecessary `cs_mains` in `denialofservice_tests` (jnewbery)
- bitcoin/bitcoin#19423 add functional test for txrelay during and after IBD (gzhao408)
- bitcoin/bitcoin#16878 Fix non-deterministic coverage of test `DoS_mapOrphans` (davereikher)
- bitcoin/bitcoin#19548 fuzz: add missing overrides to `signature_checker` (jonatack)
- bitcoin/bitcoin#19562 Fix fuzzer compilation on macOS (freenancial)
- bitcoin/bitcoin#19370 Static asserts for consistency of fee defaults (domob1812)
- bitcoin/bitcoin#19599 clean `message_count` and `last_message` (troygiorshev)
- bitcoin/bitcoin#19597 test decodepsbt fee calculation (count input value only once per UTXO) (theStack)
- bitcoin/bitcoin#18011 Replace current benchmarking framework with nanobench (martinus)
- bitcoin/bitcoin#19489 Fail `wait_until` early if connection is lost (MarcoFalke)
- bitcoin/bitcoin#19340 Preserve the `LockData` initial state if "potential deadlock detected" exception thrown (hebasto)
- bitcoin/bitcoin#19632 Catch decimal.InvalidOperation from `TestNodeCLI#send_cli` (Empact)
- bitcoin/bitcoin#19098 Remove duplicate NodeContext hacks (ryanofsky)
- bitcoin/bitcoin#19649 Restore test case for p2p transaction blinding (instagibbs)
- bitcoin/bitcoin#19657 Wait until `is_connected` in `add_p2p_connection` (MarcoFalke)
- bitcoin/bitcoin#19631 Wait for 'cmpctblock' in `p2p_compactblocks` when it is expected (Empact)
- bitcoin/bitcoin#19674 use throwaway _ variable for unused loop counters (theStack)
- bitcoin/bitcoin#19709 Fix 'make cov' with clang (hebasto)
- bitcoin/bitcoin#19564 `p2p_feefilter` improvements (logging, refactoring, speedup) (theStack)
- bitcoin/bitcoin#19756 add `sync_all` to fix race condition in wallet groups test (kallewoof)
- bitcoin/bitcoin#19727 Removing unused classes from `p2p_leak.py` (dhruv)
- bitcoin/bitcoin#19722 Add test for getblockheader verboseness (torhte)
- bitcoin/bitcoin#19659 Add a seed corpus generation option to the fuzzing `test_runner` (darosior)
- bitcoin/bitcoin#19775 Activate segwit in TestChain100Setup (MarcoFalke)
- bitcoin/bitcoin#19760 Remove confusing mininode terminology (jnewbery)
- bitcoin/bitcoin#19752 Update `wait_until` usage in tests not to use the one from utils (slmtpz)
- bitcoin/bitcoin#19839 Set appveyor VM version to previous Visual Studio 2019 release (sipsorcery)
- bitcoin/bitcoin#19830 Add tsan supp for leveldb::DBImpl::DeleteObsoleteFiles (MarcoFalke)
- bitcoin/bitcoin#19710 bench: Prevent thread oversubscription and decreases the variance of result values (hebasto)
- bitcoin/bitcoin#19842 Update the vcpkg checkout commit ID in appveyor config (sipsorcery)
- bitcoin/bitcoin#19507 Expand functional zmq transaction tests (instagibbs)
- bitcoin/bitcoin#19816 Rename wait until helper to `wait_until_helper` (MarcoFalke)
- bitcoin/bitcoin#19859 Fixes failing functional test by changing version (n-thumann)
- bitcoin/bitcoin#19887 Fix flaky `wallet_basic` test (fjahr)
- bitcoin/bitcoin#19897 Change `FILE_CHAR_BLOCKLIST` to `FILE_CHARS_DISALLOWED` (verretor)
- bitcoin/bitcoin#19800 Mockwallet (MarcoFalke)
- bitcoin/bitcoin#19922 Run `rpc_txoutproof.py` even with wallet disabled (MarcoFalke)
- bitcoin/bitcoin#19936 batch rpc with params (instagibbs)
- bitcoin/bitcoin#19971 create default wallet in extended tests (Sjors)
- bitcoin/bitcoin#19781 add parameterized constructor for `msg_sendcmpct()` (theStack)
- bitcoin/bitcoin#19963 Clarify blocksonly whitelistforcerelay test (t-bast)
- bitcoin/bitcoin#20022 Use explicit p2p objects where available (guggero)
- bitcoin/bitcoin#20028 Check that invalid peer traffic is accounted for (MarcoFalke)
- bitcoin/bitcoin#20004 Add signet witness commitment section parse tests (MarcoFalke)
- bitcoin/bitcoin#20034 Get rid of default wallet hacks (ryanofsky)
- bitcoin/bitcoin#20069 Mention commit id in scripted diff error (laanwj)
- bitcoin/bitcoin#19947 Cover `change_type` option of "walletcreatefundedpsbt" RPC (guggero)
- bitcoin/bitcoin#20126 `p2p_leak_tx.py` improvements (use MiniWallet, add `p2p_lock` acquires) (theStack)
- bitcoin/bitcoin#20129 Don't export `in6addr_loopback` (vasild)
- bitcoin/bitcoin#20131 Remove unused nVersion=1 in p2p tests (MarcoFalke)
- bitcoin/bitcoin#20161 Minor Taproot follow-ups (sipa)
- bitcoin/bitcoin#19401 Use GBT to get block versions correct (luke-jr)
- bitcoin/bitcoin#20159 `mining_getblocktemplate_longpoll.py` improvements (use MiniWallet, add logging) (theStack)
- bitcoin/bitcoin#20039 Convert amounts from float to decimal (prayank23)
- bitcoin/bitcoin#20112 Speed up `wallet_resendwallettransactions` with mockscheduler RPC (MarcoFalke)
- bitcoin/bitcoin#20247 fuzz: Check for addrv1 compatibility before using addrv1 serializer. Fuzz addrv2 serialization (practicalswift)
- bitcoin/bitcoin#20167 Add test for -blockversion (MarcoFalke)
- bitcoin/bitcoin#19877 Clarify `rpc_net` & `p2p_disconnect_ban functional` tests (amitiuttarwar)
- bitcoin/bitcoin#20258 Remove getnettotals/getpeerinfo consistency test (jnewbery)
- bitcoin/bitcoin#20242 fuzz: Properly initialize PrecomputedTransactionData (MarcoFalke)
- bitcoin/bitcoin#20262 Skip --descriptor tests if sqlite is not compiled (achow101)
- bitcoin/bitcoin#18788 Update more tests to work with descriptor wallets (achow101)
- bitcoin/bitcoin#20289 fuzz: Check for addrv1 compatibility before using addrv1 serializer/deserializer on CService (practicalswift)
- bitcoin/bitcoin#20290 fuzz: Fix DecodeHexTx fuzzing harness issue (practicalswift)
- bitcoin/bitcoin#20245 Run `script_assets_test` even if built --with-libs=no (MarcoFalke)
- bitcoin/bitcoin#20300 fuzz: Add missing `ECC_Start` to `descriptor_parse` test (S3RK)
- bitcoin/bitcoin#20283 Only try witness deser when checking for witness deser failure (MarcoFalke)
- bitcoin/bitcoin#20303 fuzz: Assert expected DecodeHexTx behaviour when using legacy decoding (practicalswift)
- bitcoin/bitcoin#20316 Fix `wallet_multiwallet` test issue on Windows (MarcoFalke)
- bitcoin/bitcoin#20326 Fix `ecdsa_verify` in test framework (stepansnigirev)
- bitcoin/bitcoin#20328 cirrus: Skip tasks on the gui repo main branch (MarcoFalke)
- bitcoin/bitcoin#20355 fuzz: Check for addrv1 compatibility before using addrv1 serializer/deserializer on CSubNet (practicalswift)
- bitcoin/bitcoin#20332 Mock IBD in `net_processing` fuzzers (MarcoFalke)
- bitcoin/bitcoin#20218 Suppress `epoll_ctl` data race (MarcoFalke)
- bitcoin/bitcoin#20375 fuzz: Improve coverage for CPartialMerkleTree fuzzing harness (practicalswift)
- bitcoin/bitcoin#19669 contrib: Fixup valgrind suppressions file (MarcoFalke)
- bitcoin/bitcoin#18879 valgrind: remove outdated suppressions (fanquake)
- bitcoin/bitcoin#19226 Add BerkeleyDatabase tsan suppression (MarcoFalke)
- bitcoin/bitcoin#20379 Remove no longer needed UBSan suppression (float divide-by-zero in validation.cpp) (practicalswift)
- bitcoin/bitcoin#18190, bitcoin/bitcoin#18736, bitcoin/bitcoin#18744, bitcoin/bitcoin#18775, bitcoin/bitcoin#18783, bitcoin/bitcoin#18867, bitcoin/bitcoin#18994, bitcoin/bitcoin#19065,
  bitcoin/bitcoin#19067, bitcoin/bitcoin#19143, bitcoin/bitcoin#19222, bitcoin/bitcoin#19247, bitcoin/bitcoin#19286, bitcoin/bitcoin#19296, bitcoin/bitcoin#19379, bitcoin/bitcoin#19934,
  #20188, bitcoin/bitcoin#20395 Add fuzzing harnessses (practicalswift)
- bitcoin/bitcoin#18638 Use mockable time for ping/pong, add tests (MarcoFalke)
- bitcoin/bitcoin#19951 CNetAddr scoped ipv6 test coverage, rename scopeId to `m_scope_id` (jonatack)
- bitcoin/bitcoin#20027 Use mockable time everywhere in `net_processing` (sipa)
- bitcoin/bitcoin#19105 Add Muhash3072 implementation in Python (fjahr)
- bitcoin/bitcoin#18704, bitcoin/bitcoin#18752, bitcoin/bitcoin#18753, bitcoin/bitcoin#18765, bitcoin/bitcoin#18839, bitcoin/bitcoin#18866, bitcoin/bitcoin#18873, bitcoin/bitcoin#19022,
  bitcoin/bitcoin#19023, bitcoin/bitcoin#19429, bitcoin/bitcoin#19552, bitcoin/bitcoin#19778, bitcoin/bitcoin#20176, bitcoin/bitcoin#20179, bitcoin/bitcoin#20214, bitcoin/bitcoin#20292,
  #20299, bitcoin/bitcoin#20322 Fix intermittent test issues (MarcoFalke)
- bitcoin/bitcoin#20390 CI/Cirrus: Skip `merge_base` step for non-PRs (luke-jr)
- bitcoin/bitcoin#18634 ci: Add fuzzbuzz integration configuration file (practicalswift)
- bitcoin/bitcoin#18591 Add C++17 build to Travis (sipa)
- bitcoin/bitcoin#18581, bitcoin/bitcoin#18667, bitcoin/bitcoin#18798, bitcoin/bitcoin#19495, bitcoin/bitcoin#19519, bitcoin/bitcoin#19538 CI improvements (hebasto)
- bitcoin/bitcoin#18683, bitcoin/bitcoin#18705, bitcoin/bitcoin#18735, bitcoin/bitcoin#18778, bitcoin/bitcoin#18799, bitcoin/bitcoin#18829, bitcoin/bitcoin#18912, bitcoin/bitcoin#18929,
  bitcoin/bitcoin#19008, bitcoin/bitcoin#19041, bitcoin/bitcoin#19164, bitcoin/bitcoin#19201, bitcoin/bitcoin#19267, bitcoin/bitcoin#19276, bitcoin/bitcoin#19321, bitcoin/bitcoin#19371,
  bitcoin/bitcoin#19427, bitcoin/bitcoin#19730, bitcoin/bitcoin#19746, bitcoin/bitcoin#19881, bitcoin/bitcoin#20294, bitcoin/bitcoin#20339, bitcoin/bitcoin#20368 CI improvements (MarcoFalke)
- bitcoin/bitcoin#20489, bitcoin/bitcoin#20506 MSVC CI improvements (sipsorcery)
- bitcoin/bitcoin#21380 Add fuzzing harness for versionbits (ajtowns)
- bitcoin/bitcoin#20812 fuzz: Bump FuzzedDataProvider.h (MarcoFalke)
- bitcoin/bitcoin#20740 fuzz: Update FuzzedDataProvider.h from upstream (LLVM) (practicalswift)
- bitcoin/bitcoin#21446 Update vcpkg checkout commit (sipsorcery)
- bitcoin/bitcoin#21397 fuzz: Bump FuzzedDataProvider.h (MarcoFalke)
- bitcoin/bitcoin#21081 Fix the unreachable code at `feature_taproot` (brunoerg)
- bitcoin/bitcoin#20562 Test that a fully signed tx given to signrawtx is unchanged (achow101)
- bitcoin/bitcoin#21571 Make sure non-IP peers get discouraged and disconnected (vasild, MarcoFalke)
- bitcoin/bitcoin#21489 fuzz: cleanups for versionbits fuzzer (ajtowns)
- bitcoin/bitcoin#20182 Build with --enable-werror by default, and document exceptions (hebasto)
- bitcoin/bitcoin#20535 Fix intermittent feature_taproot issue (MarcoFalke)
- bitcoin/bitcoin#21663 Fix macOS brew install command (hebasto)
- bitcoin/bitcoin#22279 add missing ECCVerifyHandle to base_encode_decode (apoelstra)
- bitcoin/bitcoin#22730 Run fuzzer task for the master branch only (hebasto)

### Miscellaneous
- bitcoin/bitcoin#18713 scripts: Add macho stack canary check to security-check.py (fanquake)
- bitcoin/bitcoin#18629 scripts: Add pe .reloc section check to security-check.py (fanquake)
- bitcoin/bitcoin#18437 util: `Detect posix_fallocate()` instead of assuming (vasild)
- bitcoin/bitcoin#18413 script: Prevent ub when computing abs value for num opcode serialize (pierreN)
- bitcoin/bitcoin#18443 lockedpool: avoid sensitive data in core files (FreeBSD) (vasild)
- bitcoin/bitcoin#18885 contrib: Move optimize-pngs.py script to the maintainer repo (MarcoFalke)
- bitcoin/bitcoin#18317 Serialization improvements step 6 (all except wallet/gui) (sipa)
- bitcoin/bitcoin#16127 More thread safety annotation coverage (ajtowns)
- bitcoin/bitcoin#19228 Update libsecp256k1 subtree (sipa)
- bitcoin/bitcoin#19277 util: Add assert identity function (MarcoFalke)
- bitcoin/bitcoin#19491 util: Make assert work with any value (MarcoFalke)
- bitcoin/bitcoin#19205 script: `previous_release.sh` rewritten in python (bliotti)
- bitcoin/bitcoin#15935 Add <datadir>/settings.json persistent settings storage (ryanofsky)
- bitcoin/bitcoin#19439 script: Linter to check commit message formatting (Ghorbanian)
- bitcoin/bitcoin#19654 lint: Improve commit message linter in travis (fjahr)
- bitcoin/bitcoin#15382 util: Add runcommandparsejson (Sjors)
- bitcoin/bitcoin#19614 util: Use `have_fdatasync` to determine fdatasync() use (fanquake)
- bitcoin/bitcoin#19813 util, ci: Hard code previous release tarball checksums (hebasto)
- bitcoin/bitcoin#19841 Implement Keccak and `SHA3_256` (sipa)
- bitcoin/bitcoin#19643 Add -netinfo peer connections dashboard (jonatack)
- bitcoin/bitcoin#15367 feature: Added ability for users to add a startup command (benthecarman)
- bitcoin/bitcoin#19984 log: Remove static log message "Initializing chainstate Chainstate [ibd] @ height -1 (null)" (practicalswift)
- bitcoin/bitcoin#20092 util: Do not use gargs global in argsmanager member functions (hebasto)
- bitcoin/bitcoin#20168 contrib: Fix `gen_key_io_test_vectors.py` imports (MarcoFalke)
- bitcoin/bitcoin#19624 Warn on unknown `rw_settings` (MarcoFalke)
- bitcoin/bitcoin#20257 Update secp256k1 subtree to latest master (sipa)
- bitcoin/bitcoin#20346 script: Modify security-check.py to use "==" instead of "is" for literal comparison (tylerchambers)
- bitcoin/bitcoin#18881 Prevent UB in DeleteLock() function (hebasto)
- bitcoin/bitcoin#19180, bitcoin/bitcoin#19189, bitcoin/bitcoin#19190, bitcoin/bitcoin#19220, bitcoin/bitcoin#19399 Replace RecursiveMutex with Mutex (hebasto)
- bitcoin/bitcoin#19347 Make `cs_inventory` nonrecursive (jnewbery)
- bitcoin/bitcoin#19773 Avoid recursive lock in IsTrusted (promag)
- bitcoin/bitcoin#18790 Improve thread naming (hebasto)
- bitcoin/bitcoin#20140 Restore compatibility with old CSubNet serialization (sipa)
- bitcoin/bitcoin#17775 DecodeHexTx: Try case where txn has inputs first (instagibbs)
- bitcoin/bitcoin#20861 BIP 350: Implement Bech32m and use it for v1+ segwit addresses (sipa)
- bitcoin/bitcoin#22002 Fix crash when parsing command line with -noincludeconf=0 (MarcoFalke)
- bitcoin/bitcoin#22137 util: Properly handle -noincludeconf on command line (take 2) (MarcoFalke)

### Documentation
- bitcoin/bitcoin#18502 Update docs for getbalance (default minconf should be 0) (uzyn)
- bitcoin/bitcoin#18632 Fix macos comments in release-notes (MarcoFalke)
- bitcoin/bitcoin#18645 Update thread information in developer docs (jnewbery)
- bitcoin/bitcoin#18709 Note why we can't use `thread_local` with glibc back compat (fanquake)
- bitcoin/bitcoin#18410 Improve commenting for coins.cpp|h (jnewbery)
- bitcoin/bitcoin#18157 fixing init.md documentation to not require rpcpassword (jkcd)
- bitcoin/bitcoin#18739 Document how to fuzz Bitcoin Core using Honggfuzz (practicalswift)
- bitcoin/bitcoin#18779 Better explain GNU ld's dislike of ld64's options (fanquake)
- bitcoin/bitcoin#18663 Mention build docs in README.md (saahilshangle)
- bitcoin/bitcoin#18810 Update rest info on block size and json (chrisabrams)
- bitcoin/bitcoin#18939 Add c++17-enable flag to fuzzing instructions (mzumsande)
- bitcoin/bitcoin#18957 Add a link from ZMQ doc to ZMQ example in contrib/ (meeDamian)
- bitcoin/bitcoin#19058 Drop protobuf stuff (hebasto)
- bitcoin/bitcoin#19061 Add link to Visual Studio build readme (maitrebitcoin)
- bitcoin/bitcoin#19072 Expand section on Getting Started (MarcoFalke)
- bitcoin/bitcoin#18968 noban precludes maxuploadtarget disconnects (MarcoFalke)
- bitcoin/bitcoin#19005 Add documentation for 'checklevel' argument in 'verifychain' RPC… (kcalvinalvin)
- bitcoin/bitcoin#19192 Extract net permissions doc (MarcoFalke)
- bitcoin/bitcoin#19071 Separate repository for the gui (MarcoFalke)
- bitcoin/bitcoin#19018 fixing description of the field sequence in walletcreatefundedpsbt RPC method (limpbrains)
- bitcoin/bitcoin#19367 Span pitfalls (sipa)
- bitcoin/bitcoin#19408 Windows WSL build recommendation to temporarily disable Win32 PE support (sipsorcery)
- bitcoin/bitcoin#19407 explain why passing -mlinker-version is required when cross-compiling (fanquake)
- bitcoin/bitcoin#19452 afl fuzzing comment about afl-gcc and afl-g++ (Crypt-iQ)
- bitcoin/bitcoin#19258 improve subtree check instructions (Sjors)
- bitcoin/bitcoin#19474 Use precise permission flags where possible (MarcoFalke)
- bitcoin/bitcoin#19494 CONTRIBUTING.md improvements (jonatack)
- bitcoin/bitcoin#19268 Add non-thread-safe note to FeeFilterRounder::round() (hebasto)
- bitcoin/bitcoin#19547 Update macOS cross compilation dependencies for Focal (hebasto)
- bitcoin/bitcoin#19617 Clang 8 or later is required with `FORCE_USE_SYSTEM_CLANG` (fanquake)
- bitcoin/bitcoin#19639 Remove Reference Links bitcoin/bitcoin#19582 (RobertHosking)
- bitcoin/bitcoin#19605 Set `CC_FOR_BUILD` when building on OpenBSD (fanquake)
- bitcoin/bitcoin#19765 Fix getmempoolancestors RPC result doc (MarcoFalke)
- bitcoin/bitcoin#19786 Remove label from good first issue template (MarcoFalke)
- bitcoin/bitcoin#19646 Updated outdated help command for getblocktemplate (jakeleventhal)
- bitcoin/bitcoin#18817 Document differences in bitcoind and bitcoin-qt locale handling (practicalswift)
- bitcoin/bitcoin#19870 update PyZMQ install instructions, fix `zmq_sub.py` file permissions (jonatack)
- bitcoin/bitcoin#19903 Update build-openbsd.md with GUI support (grubles)
- bitcoin/bitcoin#19241 help: Generate checkpoint height from chainparams (luke-jr)
- bitcoin/bitcoin#18949 Add CODEOWNERS file to automatically nominate PR reviewers (adamjonas)
- bitcoin/bitcoin#20014 Mention signet in -help output (hebasto)
- bitcoin/bitcoin#20015 Added default signet config for linearize script (gr0kchain)
- bitcoin/bitcoin#19958 Better document features of feelers (naumenkogs)
- bitcoin/bitcoin#19871 Clarify scope of eviction protection of outbound block-relay peers (ariard)
- bitcoin/bitcoin#20076 Update and improve files.md (hebasto)
- bitcoin/bitcoin#20107 Collect release-notes snippets (MarcoFalke)
- bitcoin/bitcoin#20109 Release notes and followups from 19339 (glozow)
- bitcoin/bitcoin#20090 Tiny followups to new getpeerinfo connection type field (amitiuttarwar)
- bitcoin/bitcoin#20152 Update wallet files in files.md (hebasto)
- bitcoin/bitcoin#19124 Document `ALLOW_HOST_PACKAGES` dependency option (skmcontrib)
- bitcoin/bitcoin#20271 Document that wallet salvage is experimental (MarcoFalke)
- bitcoin/bitcoin#20281 Correct getblockstats documentation for `(sw)total_weight` (shesek)
- bitcoin/bitcoin#20279 release process updates/fixups (jonatack)
- bitcoin/bitcoin#20238 Missing comments for signet parameters (decryp2kanon)
- bitcoin/bitcoin#20756 Add missing field (permissions) to the getpeerinfo help (amitiuttarwar)
- bitcoin/bitcoin#20668 warn that incoming conns are unlikely when not using default ports (adamjonas)
- bitcoin/bitcoin#19961 tor.md updates (jonatack)
- bitcoin/bitcoin#19050 Add warning for rest interface limitation (fjahr)
- bitcoin/bitcoin#19390 doc/REST-interface: Remove stale info (luke-jr)
- bitcoin/bitcoin#19344 docs: update testgen usage example (Bushstar)
- bitcoin/bitcoin#21384 add signet to bitcoin.conf documentation (jonatack)
- bitcoin/bitcoin#21342 Remove outdated comment (hebasto)

Credits
-------

Thanks to everyone who directly contributed to this release:

- 10xcryptodev
- Aaron Clauson
- Aaron Hook
- Adam Jonas
- Adam Soltys
- Adam Stein
- Akio Nakamura
- Alex Willmer
- Amir Ghorbanian
- Amiti Uttarwar
- Andrew Chow
- Andrew Poelstra
- Andrew Toth
- Anthony Fieroni
- Anthony Towns
- Antoine Poinsot
- Antoine Riard
- Ben Carman
- Ben Woosley
- Benoit Verret
- Brian Liotti
- Bruno Garcia
- Bushstar
- Calvin Kim
- Carl Dong
- Chris Abrams
- Chris L
- Christopher Coverdale
- codeShark149
- Cory Fields
- Craig Andrews
- Damian Mee
- Daniel Kraft
- Danny Lee
- David Reikher
- DesWurstes
- Dhruv Mehta
- Duncan Dean
- Elichai Turkel
- Elliott Jin
- Emil Engler
- Ethan Heilman
- eugene
- Fabian Jahr
- fanquake
- Ferdinando M. Ametrano
- freenancial
- furszy
- Gillian Chu
- Gleb Naumenko
- Glenn Willen
- Gloria Zhao
- glowang
- gr0kchain
- Gregory Sanders
- grubles
- gzhao408
- Harris
- Hennadii Stepanov
- Hugo Nguyen
- Igor Cota
- Ivan Metlushko
- Ivan Vershigora
- Jake Leventhal
- James O'Beirne
- Jeremy Rubin
- jgmorgan
- Jim Posen
- jkcd
- jmorgan
- João Barbosa
- John Newbery
- Johnson Lau
- Jon Atack
- Jonas Schnelli
- Jonathan Schoeller
- Justin Moon
- kanon
- Karl-Johan Alm
- Kiminuo
- Kittywhiskers Van Gogh
- Kristaps Kaupe
- lontivero
- Luke Dashjr
- Marcin Jachymiak
- MarcoFalke
- Mark Friedenbach
- Martin Ankerl
- Martin Zumsande
- maskoficarus
- Matt Corallo
- Matthew Zipkin
- MeshCollider
- Miguel Herranz
- MIZUTA Takeshi
- mruddy
- Nadav Ivgi
- Neha Narula
- Nicolas Thumann
- Niklas Gögge
- Nima Yazdanmehr
- nsa
- nthumann
- Oliver Gugger
- pad
- pasta
- Pavol Rusnak
- Peter Bushnell
- pierrenn
- Pieter Wuille
- practicalswift
- Prayank
- prayank23
- Rafael Sadowski
- RandyMcMillan
- randymcmillan
- Raúl Martínez (RME)
- Rene Pickhardt
- Riccardo Masutti
- Robert
- Rod Vagg
- Roy Shao
- Russell Yanofsky
- Saahil Shangle
- sachinkm77
- saibato
- Samuel Dobson
- sanket1729
- Sebastian Falbesoner
- Seleme Topuz
- Sishir Giri
- Sjors Provoost
- skmcontrib
- Stepan Snigirev
- Stephan Oeste
- Suhas Daftuar
- t-bast
- Tom Harding
- Torhte Butler
- TrentZ
- Troy Giorshev
- tryphe
- Tyler Chambers
- U-Zyn Chua
- Vasil Dimov
- W. J. van der Laan
- wiz
- Wladimir J. van der Laan

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
