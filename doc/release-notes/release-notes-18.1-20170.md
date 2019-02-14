v18.1-20170 Release Notes
=========================

Freicoin version v18.1-20170 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v18.1-20170

This is a new major version release, including new features, various bug fixes and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

How to Upgrade
--------------

If you are running an older version, shut it down.  Wait until it has completely shut down (which might take a few minutes for older versions), then run the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on Mac) or `freicoind`/`freicoin-qt` (on Linux).

If your node has a txindex, the txindex db will be migrated the first time you run v17.2-18404 or newer, which may take up to a few hours.  Your node will not be functional until this migration completes.

The first time you run version v15.2-15124 or newer, your chainstate database will be converted to a new format, which will take anywhere from a few minutes to half an hour, depending on the speed of your machine.

Compatibility
-------------

Freicoin is supported and extensively tested on operating systems using the Linux kernel, macOS 10.10+, and Windows 7 and newer.  It is not recommended to use Freicoin on unsupported systems.

Freicoin should also work on most other Unix-like systems but is not as frequently tested on them.

From v17.2-18404 onwards, macOS <10.10 is no longer supported.  v17.2-18404 is built using Qt 5.9.x, which doesn't support versions of macOS older than 10.10.  Additionally, Freicoin does not yet change appearance when macOS "dark mode" is activated.

In addition to previously-supported CPU platforms, this release's pre-compiled distribution also provides binaries for the RISC-V platform.

If you are using the `systemd` unit configuration file located at `contrib/init/freicoind.service`, it has been changed to use `/var/lib/freicoind` as the data directory instead of `~freicoin/.freicoin`.  When switching over to the new configuration file, please make sure that the filesystem on which `/var/lib/freicoind` will exist has enough space (check using `df -h /var/lib/freicoind`), and optionally copy over your existing data directory.  See the [systemd init file section](#systemd-init-file) for more details.

Known issues
------------

### Wallet GUI

For advanced users who have both (1) enabled coin control features, and (2) are using multiple wallets loaded at the same time: The coin control input selection dialog can erroneously retain wrong-wallet state when switching wallets using the dropdown menu.  For now, it is recommended not to use coin control features with multiple wallets loaded.

Notable changes
---------------

### Mining

- Calls to `getblocktemplate` will fail if the segwit rule is not specified. Calling `getblocktemplate` without segwit specified is almost certainly a misconfiguration since the miner must make segwit commitments for their blocks to be valid.  Failed calls will produce an error message describing how to enable the segwit rule.

### Configuration option changes

- A warning is printed if an unrecognized section name is used in the configuration file.  Recognized sections are `[test]`, `[main]`, and `[regtest]`.

- Four new options are available for configuring the maximum number of messages that ZMQ will queue in memory (the "high water mark") before dropping additional messages.  The default value is 1,000, the same as was used for previous releases.  See the [ZMQ documentation](https://github.com/tradecraftio/tradecraft/blob/master/doc/zmq.md#usage) for details.

- The `rpcallowip` option can no longer be used to automatically listen on all network interfaces.  Instead, the `rpcbind` parameter must be used to specify the IP addresses to listen on.  Listening for RPC commands over a public network connection is insecure and should be disabled, so a warning is now printed if a user selects such a configuration.  If you need to expose RPC in order to use a tool like Docker, ensure you only bind RPC to your localhost, e.g. `docker run [...] -p 127.0.0.1:8638:8638` (this is an extra `:8638` over the normal Docker port specification).

- The `rpcpassword` option now causes a startup error if the password set in the configuration file contains a hash character (#), as it's ambiguous whether the hash character is meant for the password or as a comment.

- The `whitelistforcerelay` option is used to relay transactions from whitelisted peers even when not accepted to the mempool.  This option now defaults to being off, so that changes in policy and disconnect/ban behavior will not cause a node that is whitelisting another to be dropped by peers.  Users can still explicitly enable this behavior with the command line option (and may want to consider contacting the Freicoin project to let us know about their use-case, as this feature could be deprecated in the future).

### systemd init file

The systemd init file (`contrib/init/freicoind.service`) has been changed to use `/var/lib/freicoind` as the data directory instead of `~freicoin/.freicoin`. This change makes Freicoin more consistent with other services, and makes the systemd init config more consistent with existing Upstart and OpenRC configs.

The configuration, PID, and data directories are now completely managed by systemd, which will take care of their creation, permissions, etc.  See [`systemd.exec(5)`](https://www.freedesktop.org/software/systemd/man/systemd.exec.html#RuntimeDirectory=) for more details.

When using the provided init files under `contrib/init`, overriding the `datadir` option in `/etc/freicoin/freicoin.conf` will have no effect.  This is because the command line arguments specified in the init files take precedence over the options specified in `/etc/freicoin/freicoin.conf`.

### Documentation

- A new short [document](https://github.com/tradecraftio/tradecraft/blob/master/doc/JSON-RPC-interface.md) about the JSON-RPC interface describes cases where the results of an RPC might contain inconsistencies between data sourced from different subsystems, such as wallet state and mempool state.  A note is added to the [REST interface documentation](https://github.com/tradecraftio/tradecraft/blob/master/doc/REST-interface.md) indicating that the same rules apply.

- Further information is added to the [JSON-RPC documentation](https://github.com/tradecraftio/tradecraft/blob/master/doc/JSON-RPC-interface.md) about how to secure this interface.

- A new [document](https://github.com/tradecraftio/tradecraft/blob/master/doc/bitcoin-conf.md) about the `freicoin.conf` file describes how to use it to configure Freicoin.

- A new document introduces Freicoin's BIP174-based [Partially-Signed Transactions (PST)](https://github.com/tradecraftio/tradecraft/blob/master/doc/pst.md) interface, which is used to allow multiple programs to collaboratively work to create, sign, and broadcast new transactions.  This is useful for offline (cold storage) wallets, multisig wallets, coinjoin implementations, and many other cases where two or more programs need to interact to generate a complete transaction.

- The [output script descriptor](https://github.com/tradecraftio/tradecraft/blob/master/doc/descriptors.md) documentation has been updated with information about new features in this still-developing language for describing the output scripts that a wallet or other program wants to receive notifications for, such as which addresses it wants to know received payments.  The language is currently used in multiple new and updated RPCs described in these release notes and is expected to be adapted to other RPCs and to the underlying wallet structure.

### Build system changes

- A new `--disable-bip70` option may be passed to `./configure` to prevent Freicoin-Qt from being built with support for the BIP70 payment protocol or from linking libssl.  As the payment protocol has exposed Freicoin to libssl vulnerabilities in the past, builders who don't need BIP70 support are encouraged to use this option to reduce their exposure to future vulnerabilities.

- The minimum required version of Qt (when building the GUI) has been increased from 5.2 to 5.5.1 (the [depends system](https://github.com/tradecraftio/tradecraft/blob/master/depends/README.md) provides 5.9.7)

### New RPCs

- `getnodeaddresses` returns peer addresses known to this node. It may be used to find nodes to connect to without using a DNS seeder.

- `listwalletdir` returns a list of wallets in the wallet directory (either the default wallet directory or the directory configured by the `-walletdir` parameter).

- `getrpcinfo` returns runtime details of the RPC server. At the moment, it returns an array of the currently active commands and how long they've been running.

- `deriveaddresses` returns one or more addresses corresponding to an [output descriptor](https://github.com/tradecraftio/tradecraft/blob/master/doc/descriptors.md).

- `getdescriptorinfo` accepts a descriptor and returns information about it, including its computed checksum.

- `joinpsts` merges multiple distinct PSTs into a single PST. The multiple PSTs must have different inputs.  The resulting PST will contain every input and output from all of the PSTs.  Any signatures provided in any of the PSTs will be dropped.

- `analyzepst` examines a PST and provides information about what the PST contains and the next steps that need to be taken in order to complete the transaction.  For each input of a PST, `analyzepst` provides information about what information is missing for that input, including whether a UTXO needs to be provided, what pubkeys still need to be provided, which scripts need to be provided, and what signatures are still needed.  Every input will also list which role is needed to complete that input, and `analyzepst` will also list the next role in general needed to complete the PST.  `analyzepst` will also provide the estimated fee rate and estimated virtual size of the completed transaction if it has enough information to do so.

- `utxoupdatepst` searches the set of Unspent Transaction Outputs (UTXOs) to find the outputs being spent by the partial transaction.  PSTs need to have the UTXOs being spent to be provided because the signing algorithm requires information from the UTXO being spent.  For segwit inputs, only the UTXO itself is necessary.  For non-segwit outputs, the entire previous transaction is needed so that signers can be sure that they are signing the correct thing.  Unfortunately, because the UTXO set only contains UTXOs and not full transactions, `utxoupdatepst` will only add the UTXO for segwit inputs.

### Updated RPCs

Note: some low-level RPC changes mainly useful for testing are described in the Low-level Changes section below.

- `getpeerinfo` now returns an additional `minfeefilter` field set to the peer's BIP133 fee filter.  You can use this to detect that you have peers that are willing to accept transactions below the default minimum relay fee.

- `settxfee` previously silently ignored attempts to set the fee below the allowed minimums.  It now prints a warning.  The special value of "0" may still be used to request the minimum value.

- `getaddressinfo` now provides an `ischange` field indicating whether the wallet used the address in a change output.

- `importmulti` has been updated to support P2WSH and P2WPK.  Requests for P2WSH and P2WPK accept an additional `witnessscript` parameter.

- `importmulti` now returns an additional `warnings` field for each request with an array of strings explaining when fields are being ignored or are inconsistent, if there are any.

- `getaddressinfo` now returns an additional `solvable` boolean field when Freicoin knows enough about the address's scriptPubKey, optional redeemScript, and optional witnessScript in order for the wallet to be able to generate an unsigned input spending funds sent to that address.

- The `getaddressinfo`, `listunspent`, and `scantxoutset` RPCs now return an additional `desc` field that contains an output descriptor containing all key paths and signing information for the address (except for the private key).  The `desc` field is only returned for `getaddressinfo` and `listunspent` when the address is solvable.

- `importprivkey` will preserve previously-set labels for addresses or public keys corresponding to the private key being imported.  For example, if you imported a watch-only address with the label "cold wallet" in earlier releases of Freicoin, subsequently importing the private key would default to resetting the address's label to the default empty-string label ("").  In this release, the previous label of "cold wallet" will be retained.  If you optionally specify any label besides the default when calling `importprivkey`, the new label will be applied to the address.

- See the [Mining](#mining) section for changes to `getblocktemplate`.

- `getmininginfo` now omits `currentblockweight` and `currentblocktx` when a block was never assembled via RPC on this node.

- The `getrawtransaction` RPC & REST endpoints no longer check the unspent UTXO set for a transaction. The remaining behaviors are as follows:
    1. If a blockhash is provided, check the corresponding block.
    2. If no blockhash is provided, check the mempool.
    3. If no blockhash is provided but txindex is enabled, also check txindex.

- `unloadwallet` is now synchronous, meaning it will not return until the wallet is fully unloaded.

- `importmulti` now supports importing of addresses from descriptors.  A "desc" parameter can be provided instead of the "scriptPubKey" in a request, as well as an optional range for ranged descriptors to specify the start and end of the range to import.  Descriptors with key origin information imported through `importmulti` will have their key origin information stored in the wallet for use with creating PSTs.  More information about descriptors can be found [here](https://github.com/tradecraftio/tradecraft/blob/master/doc/descriptors.md).

- `listunspent` has been modified so that it also returns `witnessScript`, the witness script in the case of a P2WSH or P2WPK output.

- `createwallet` now has an optional `blank` argument that can be used to create a blank wallet.  Blank wallets do not have any keys or HD seed.  They cannot be opened in software older than v18.1-20170.  Once a blank wallet has a HD seed set (by using `sethdseed`) or private keys, scripts, addresses, and other watch only things have been imported, the wallet is no longer blank and can be opened in v17.  Encrypting a blank wallet will also set a HD seed for it.

### Deprecated or removed RPCs

- `signrawtransaction` is removed after being deprecated and hidden behind a special configuration option in version v17.2-18404.

- The 'account' API is removed after being deprecated in v17.2-18404.  The 'label' API was introduced in v17.2-18404 as a replacement for accounts.  See the [release notes from v17.2-18404](https://github.com/tradecraftio/tradecraft/blob/master/doc/release-notes/release-notes-17.2-18404.md#label-and-account-apis-for-wallet) for a full description of the changes from the 'account' API to the 'label' API.

- `addwitnessaddress` is removed after being deprecated in version v16.3-16386.

- `generate` is deprecated and will be fully removed in a subsequent major version.  This RPC is only used for testing, but its implementation reached across multiple subsystems (wallet and mining), so it is being deprecated to simplify the wallet-node interface.  Projects that are using `generate` for testing purposes should transition to using the `generatetoaddress` RPC, which does not require or use the wallet component.  Calling `generatetoaddress` with an address returned by the `getnewaddress` RPC gives the same functionality as the old `generate` RPC.  To continue using `generate` in this version, restart freicoind with the `-deprecatedrpc=generate` configuration option.

- Be reminded that parts of the `validateaddress` command have been deprecated and moved to `getaddressinfo`.  The following deprecated fields have moved to `getaddressinfo`: `ismine`, `iswatchonly`, `script`, `hex`, `pubkeys`, `sigsrequired`, `pubkey`, `embedded`, `iscompressed`, `label`, `timestamp`, `hdkeypath`, `hdmasterkeyid`.

- The `addresses` field has been removed from the `validateaddress` and `getaddressinfo` RPC methods.  This field was confusing since it referred to public keys using their P2PKH address.  Clients should use the `embedded.address` field for P2SH, P2WSH, or P2WPK wrapped addresses, and `pubkeys` for inspecting multisig participants.

### REST changes

- A new `/rest/blockhashbyheight/` endpoint is added for fetching the hash of the block in the current best blockchain based on its height (how many blocks it is after the Genesis Block).

### Graphical User Interface (GUI)

- A new Window menu is added alongside the existing File, Settings, and Help menus.  Several items from the other menus that opened new windows have been moved to this new Window menu.

- In the Send tab, the checkbox for "pay only the required fee" has been removed.  Instead, the user can simply decrease the value in the Custom Feerate field all the way down to the node's configured minimum relay fee.

- In the Overview tab, the watch-only balance will be the only balance shown if the wallet was created using the `createwallet` RPC and the `disable_private_keys` parameter was set to true.

- The launch-on-startup option is no longer available on macOS if compiled with macosx min version greater than 10.11 (use CXXFLAGS="-mmacosx-version-min=10.11" CFLAGS="-mmacosx-version-min=10.11" for setting the deployment sdk version)

### Tools

- A new `freicoin-wallet` tool is now distributed alongside Freicoin's other executables.  Without needing to use any RPCs, this tool can currently create a new wallet file or display some basic information about an existing wallet, such as whether the wallet is encrypted, whether it uses an HD seed, how many transactions it contains, and how many address book entries it has.

Planned changes
---------------

This section describes planned changes to Freicoin that may affect other Freicoin software and services.

### Deprecated P2P messages

- BIP 61 reject messages are now deprecated. Reject messages have no use case on the P2P network and are only logged for debugging by most network nodes.  Furthermore, they increase bandwidth and can be harmful for privacy and security.  It has been possible to disable BIP 61 messages since v17.2-18404 with the `-enablebip61=0` option.  BIP 61 messages will be disabled by default in a future version, before being removed entirely.

Low-level changes
-----------------

This section describes RPC changes mainly useful for testing, mostly not relevant in production. The changes are mentioned for completeness.

### RPC

- The `submitblock` RPC previously returned the reason a rejected block was invalid the first time it processed that block, but returned a generic "duplicate" rejection message on subsequent occasions it processed the same block.  It now always returns the fundamental reason for rejecting an invalid block and only returns "duplicate" for valid blocks it has already accepted.

- A new `submitheader` RPC allows submitting block headers independently from their block.  This is likely only useful for testing.

- The `signrawtransactionwithkey` and `signrawtransactionwithwallet` RPCs have been modified so that they also optionally accept a `witnessScript`, the witness script in the case of a P2WSH or P2SH-P2WSH output. This is compatible with the change to `listunspent`.

- For the `walletprocesspst` and `walletcreatefundedpst` RPCs, if the `bip32derivs` parameter is set to true but the key metadata for a public key has not been updated yet, then that key will have a derivation path as if it were just an independent key (i.e. no derivation path and its master fingerprint is itself).

### Configuration

- The `-usehd` configuration option was removed in version v16.3-16386.  From that version onwards, all new wallets created are hierarchical deterministic wallets.  This release makes specifying `-usehd` an invalid configuration option.

### Network

- This release allows peers that your node automatically disconnected for misbehavior (e.g. sending invalid data) to reconnect to your node if you have unused incoming connection slots.  If your slots fill up, a misbehaving node will be disconnected to make room for nodes without a history of problems (unless the misbehaving node helps your node in some other way, such as by connecting to a part of the Internet from which you don't have many other peers).  Previously, Freicoin banned the IP addresses of misbehaving peers for a period of time (default of 1 day); this was easily circumvented by attackers with multiple IP addresses. If you manually ban a peer, such as by using the `setban` RPC, all connections from that peer will still be rejected.

### Wallet

- The key metadata will need to be upgraded the first time that the HD seed is available.  For unencrypted wallets this will occur on wallet loading.  For encrypted wallets this will occur the first time the wallet is unlocked.

- Newly encrypted wallets will no longer require restarting the software. Instead such wallets will be completely unloaded and reloaded to achieve the same effect.

### Security

- This release changes the Random Number Generator (RNG) used from OpenSSL to Freicoin's own implementation, although entropy gathered by Freicoin is fed out to OpenSSL and then read back in when the program needs strong randomness.  This moves Freicoin a little closer to no longer needing to depend on OpenSSL, a dependency that has caused security issues in the past.  The new implementation gathers entropy from multiple sources, including from hardware supporting the rdseed CPU instruction.

### Changes for particular platforms

- On macOS, Freicoin now opts out of application CPU throttling ("app nap") during initial blockchain download, when catching up from over 100 blocks behind the current chain tip, or when reindexing chain data.  This helps prevent these operations from taking an excessively long time because the operating system is attempting to conserve power.

v18.1-20170 Change log
======================

### Consensus
- bitcoin/bitcoin#14247 Fix crash bug with duplicate inputs within a transaction (TheBlueMatt)

### Mining
- bitcoin/bitcoin#14811 Mining: Enforce that segwit option must be set in GBT (jnewbery)

### Block and transaction handling
- bitcoin/bitcoin#13310 Report progress in ReplayBlocks while rolling forward (promag)
- bitcoin/bitcoin#13783 validation: Pass tx pool reference into CheckSequenceLocks (MarcoFalke)
- bitcoin/bitcoin#14834 validation: Assert that pindexPrev is non-null when required (kallewoof)
- bitcoin/bitcoin#14085 index: Fix for indexers skipping genesis block (jimpo)
- bitcoin/bitcoin#14963 mempool, validation: Explain `cs_main` locking semantics (MarcoFalke)
- bitcoin/bitcoin#15193 Default `-whitelistforcerelay` to off (sdaftuar)
- bitcoin/bitcoin#15429 Update `assumevalid`, `minimumchainwork`, and `getchaintxstats` to height 563378 (gmaxwell)
- bitcoin/bitcoin#15552 Granular invalidateblock and RewindBlockIndex (MarcoFalke)
- bitcoin/bitcoin#14841 Move CheckBlock() call to critical section (hebasto)

### P2P protocol and network code
- bitcoin/bitcoin#14025 Remove dead code for nVersion=10300 (MarcoFalke)
- bitcoin/bitcoin#12254 BIP 158: Compact Block Filters for Light Clients (jimpo)
- bitcoin/bitcoin#14073 blockfilter: Avoid out-of-bounds script access (jimpo)
- bitcoin/bitcoin#14140 Switch nPrevNodeCount to vNodesSize (pstratem)
- bitcoin/bitcoin#14027 Skip stale tip checking if outbound connections are off or if reindexing (gmaxwell)
- bitcoin/bitcoin#14532 Never bind `INADDR_ANY` by default, and warn when doing so explicitly (luke-jr)
- bitcoin/bitcoin#14733 Make peer timeout configurable, speed up very slow test and ensure correct code path tested (zallarak)
- bitcoin/bitcoin#14336 Implement poll (pstratem)
- bitcoin/bitcoin#15051 IsReachable is the inverse of IsLimited (DRY). Includes unit tests (mmachicao)
- bitcoin/bitcoin#15138 Drop IsLimited in favor of IsReachable (Empact)
- bitcoin/bitcoin#14605 Return of the Banman (dongcarl)
- bitcoin/bitcoin#14970 Add dnsseed.emzy.de to DNS seeds (Emzy)
- bitcoin/bitcoin#14929 Allow connections from misbehavior banned peers (gmaxwell)
- bitcoin/bitcoin#15345 Correct comparison of addr count (dongcarl)
- bitcoin/bitcoin#15201 Add missing locking annotation for vNodes. vNodes is guarded by cs_vNodes (practicalswift)
- bitcoin/bitcoin#14626 Select orphan transaction uniformly for eviction (sipa)
- bitcoin/bitcoin#15486 Ensure tried collisions resolve, and allow feeler connections to existing outbound netgroups (sdaftuar)
- bitcoin/bitcoin#15990 Add tests and documentation for blocksonly (MarcoFalke)
- bitcoin/bitcoin#16021 Avoid logging transaction decode errors to stderr (MarcoFalke)
- bitcoin/bitcoin#16405 fix: tor: Call `event_base_loopbreak` from the event's callback (promag)
- bitcoin/bitcoin#16412 Make poll in InterruptibleRecv only filter for POLLIN events (tecnovert)

### Wallet
- bitcoin/bitcoin#13962 Remove unused `dummy_tx` variable from FillPSBT (dongcarl)
- bitcoin/bitcoin#13967 Don't report `minversion` wallet entry as unknown (instagibbs)
- bitcoin/bitcoin#13988 Add checks for settxfee reasonableness (ajtowns)
- bitcoin/bitcoin#12559 Avoid locking `cs_main` in some wallet RPC (promag)
- bitcoin/bitcoin#13631 Add CMerkleTx::IsImmatureCoinBase method (Empact)
- bitcoin/bitcoin#14023 Remove accounts RPCs (jnewbery)
- bitcoin/bitcoin#13825 Kill accounts (jnewbery)
- bitcoin/bitcoin#10605 Add AssertLockHeld assertions in CWallet::ListCoins (ryanofsky)
- bitcoin/bitcoin#12490 Remove deprecated wallet rpc features from `bitcoin_server` (jnewbery)
- bitcoin/bitcoin#14138 Set `encrypted_batch` to nullptr after delete. Avoid double free in the case of NDEBUG (practicalswift)
- bitcoin/bitcoin#14168 Remove `ENABLE_WALLET` from `libbitcoin_server.a` (jnewbery)
- bitcoin/bitcoin#12493 Reopen CDBEnv after encryption instead of shutting down (achow101)
- bitcoin/bitcoin#14282 Remove `-usehd` option (jnewbery)
- bitcoin/bitcoin#14146 Remove trailing separators from `-walletdir` arg (PierreRochard)
- bitcoin/bitcoin#14291 Add ListWalletDir utility function (promag)
- bitcoin/bitcoin#14468 Deprecate `generate` RPC method (jnewbery)
- bitcoin/bitcoin#11634 Add missing `cs_wallet`/`cs_KeyStore` locks to wallet (practicalswift)
- bitcoin/bitcoin#14296 Remove `addwitnessaddress` (jnewbery)
- bitcoin/bitcoin#14451 Add BIP70 deprecation warning and allow building GUI without BIP70 support (jameshilliard)
- bitcoin/bitcoin#14320 Fix duplicate fileid detection (ken2812221)
- bitcoin/bitcoin#14561 Remove `fs::relative` call and fix listwalletdir tests (promag)
- bitcoin/bitcoin#14454 Add SegWit support to importmulti (MeshCollider)
- bitcoin/bitcoin#14410 rpcwallet: `ischange` field for `getaddressinfo` RPC (mrwhythat)
- bitcoin/bitcoin#14350 Add WalletLocation class (promag)
- bitcoin/bitcoin#14689 Require a public key to be retrieved when signing a P2PKH input (achow101)
- bitcoin/bitcoin#14478 Show error to user when corrupt wallet unlock fails (MeshCollider)
- bitcoin/bitcoin#14411 Restore ability to list incoming transactions by label (ryanofsky)
- bitcoin/bitcoin#14552 Detect duplicate wallet by comparing the db filename (ken2812221)
- bitcoin/bitcoin#14678 Remove redundant KeyOriginInfo access, already done in CreateSig (instagibbs)
- bitcoin/bitcoin#14477 Add ability to convert solvability info to descriptor (sipa)
- bitcoin/bitcoin#14380 Fix assert crash when specified change output spend size is unknown (instagibbs)
- bitcoin/bitcoin#14760 Log env path in `BerkeleyEnvironment::Flush` (promag)
- bitcoin/bitcoin#14646 Add expansion cache functions to descriptors (unused for now) (sipa)
- bitcoin/bitcoin#13076 Fix ScanForWalletTransactions to return an enum indicating scan result: `success` / `failure` / `user_abort` (Empact)
- bitcoin/bitcoin#14821 Replace CAffectedKeysVisitor with descriptor based logic (sipa)
- bitcoin/bitcoin#14957 Initialize `stop_block` in CWallet::ScanForWalletTransactions (Empact)
- bitcoin/bitcoin#14565 Overhaul `importmulti` logic (sipa)
- bitcoin/bitcoin#15039 Avoid leaking nLockTime fingerprint when anti-fee-sniping (MarcoFalke)
- bitcoin/bitcoin#14268 Introduce SafeDbt to handle Dbt with free or `memory_cleanse` raii-style (Empact)
- bitcoin/bitcoin#14711 Remove uses of chainActive and mapBlockIndex in wallet code (ryanofsky)
- bitcoin/bitcoin#15279 Clarify rescanblockchain doc (MarcoFalke)
- bitcoin/bitcoin#15292 Remove `boost::optional`-related false positive -Wmaybe-uninitialized warnings on GCC compiler (hebasto)
- bitcoin/bitcoin#13926 [Tools] bitcoin-wallet - a tool for creating and managing wallets offline (jnewbery)
- bitcoin/bitcoin#11911 Free BerkeleyEnvironment instances when not in use (ryanofsky)
- bitcoin/bitcoin#15235 Do not import private keys to wallets with private keys disabled (achow101)
- bitcoin/bitcoin#15263 Descriptor expansions only need pubkey entries for PKH/WPKH (sipa)
- bitcoin/bitcoin#15322 Add missing `cs_db` lock (promag)
- bitcoin/bitcoin#15297 Releases dangling files on `BerkeleyEnvironment::Close` (promag)
- bitcoin/bitcoin#14491 Allow descriptor imports with importmulti (MeshCollider)
- bitcoin/bitcoin#15365 Add lock annotation for mapAddressBook (MarcoFalke)
- bitcoin/bitcoin#15226 Allow creating blank (empty) wallets (alternative) (achow101)
- bitcoin/bitcoin#15390 [wallet-tool] Close bdb when flushing wallet (jnewbery)
- bitcoin/bitcoin#15334 Log absolute paths for the wallets (hebasto)
- bitcoin/bitcoin#14978 Factor out PSBT utilities from RPCs for use in GUI code; related refactoring (gwillen)
- bitcoin/bitcoin#14481 Add P2SH-P2WSH support to listunspent RPC (MeshCollider)
- bitcoin/bitcoin#14021 Import key origin data through descriptors in importmulti (achow101)
- bitcoin/bitcoin#14075 Import watch only pubkeys to the keypool if private keys are disabled (achow101)
- bitcoin/bitcoin#15368 Descriptor checksums (sipa)
- bitcoin/bitcoin#15433 Use a single wallet batch for `UpgradeKeyMetadata` (jonasschnelli)
- bitcoin/bitcoin#15408 Remove unused `TransactionError` constants (MarcoFalke)
- bitcoin/bitcoin#15583 Log and ignore errors in ListWalletDir and IsBerkeleyBtree (promag)
- bitcoin/bitcoin#14195 Pass privkey export DER compression flag correctly (fingera)
- bitcoin/bitcoin#15299 Fix assertion in `CKey::SignCompact` (promag)
- bitcoin/bitcoin#14437 Start to separate wallet from node (ryanofsky)
- bitcoin/bitcoin#15749 Fix: importmulti only imports origin info for PKH outputs (sipa)
- bitcoin/bitcoin#15913 Add -ignorepartialspends to list of ignored wallet options (luke-jr)

### RPC and other APIs
- bitcoin/bitcoin#12842 Prevent concurrent `savemempool` (promag)
- bitcoin/bitcoin#13987 Report `minfeefilter` value in `getpeerinfo` RPC (ajtowns)
- bitcoin/bitcoin#13891 Remove getinfo deprecation warning (jnewbery)
- bitcoin/bitcoin#13399 Add `submitheader` (MarcoFalke)
- bitcoin/bitcoin#12676 Show `bip125-replaceable` flag, when retrieving mempool entries (dexX7)
- bitcoin/bitcoin#13723 PSBT key path cleanups (sipa)
- bitcoin/bitcoin#14008 Preserve a format of RPC command definitions (kostyantyn)
- bitcoin/bitcoin#9332 Let wallet `importmulti` RPC accept labels for standard scriptPubKeys (ryanofsky)
- bitcoin/bitcoin#13983 Return more specific reject reason for submitblock (MarcoFalke)
- bitcoin/bitcoin#13152 Add getnodeaddresses RPC command (chris-belcher)
- bitcoin/bitcoin#14298 rest: Improve performance for JSON calls (alecalve)
- bitcoin/bitcoin#14297 Remove warning for removed estimatefee RPC (jnewbery)
- bitcoin/bitcoin#14373 Consistency fixes for RPC descriptions (ch4ot1c)
- bitcoin/bitcoin#14150 Add key origin support to descriptors (sipa)
- bitcoin/bitcoin#14518 Always throw in getblockstats if `-txindex` is required (promag)
- bitcoin/bitcoin#14060 ZMQ: add options to configure outbound message high water mark, aka SNDHWM (mruddy)
- bitcoin/bitcoin#13381 Add possibility to preserve labels on importprivkey (marcoagner)
- bitcoin/bitcoin#14530 Use `RPCHelpMan` to generate RPC doc strings (MarcoFalke)
- bitcoin/bitcoin#14720 Correctly name RPC arguments (MarcoFalke)
- bitcoin/bitcoin#14726 Use `RPCHelpMan` for all RPCs (MarcoFalke)
- bitcoin/bitcoin#14796 Pass argument descriptions to `RPCHelpMan` (MarcoFalke)
- bitcoin/bitcoin#14670 http: Fix HTTP server shutdown (promag)
- bitcoin/bitcoin#14885 Assert that named arguments are unique in `RPCHelpMan` (promag)
- bitcoin/bitcoin#14877 Document default values for optional arguments (MarcoFalke)
- bitcoin/bitcoin#14875 RPCHelpMan: Support required arguments after optional ones (MarcoFalke)
- bitcoin/bitcoin#14993 Fix data race (UB) in InterruptRPC() (practicalswift)
- bitcoin/bitcoin#14653 rpcwallet: Add missing transaction categories to RPC helptexts (andrewtoth)
- bitcoin/bitcoin#14981 Clarify RPC `getrawtransaction`'s time help text (benthecarman)
- bitcoin/bitcoin#12151 Remove `cs_main` lock from blockToJSON and blockheaderToJSON (promag)
- bitcoin/bitcoin#15078 Document `bytessent_per_msg` and `bytesrecv_per_msg` (MarcoFalke)
- bitcoin/bitcoin#15057 Correct `reconsiderblock `help text, add test (MarcoFalke)
- bitcoin/bitcoin#12153 Avoid permanent `cs_main` lock in `getblockheader` (promag)
- bitcoin/bitcoin#14982 Add `getrpcinfo` command (promag)
- bitcoin/bitcoin#15122 Expand help text for `importmulti` changes (jnewbery)
- bitcoin/bitcoin#15186 remove duplicate solvable field from `getaddressinfo` (fanquake)
- bitcoin/bitcoin#15209 zmq: log outbound message high water mark when reusing socket (fanquake)
- bitcoin/bitcoin#15177 rest: Improve tests and documention of /headers and /block (promag)
- bitcoin/bitcoin#14353 rest: Add blockhash call, fetch blockhash by height (jonasschnelli)
- bitcoin/bitcoin#15248 Compile on GCC4.8 (MarcoFalke)
- bitcoin/bitcoin#14987 RPCHelpMan: Pass through Result and Examples (MarcoFalke)
- bitcoin/bitcoin#15159 Remove lookup to UTXO set from GetTransaction (amitiuttarwar)
- bitcoin/bitcoin#15245 remove deprecated mentions of signrawtransaction from fundraw help (instagibbs)
- bitcoin/bitcoin#14667 Add `deriveaddresses` RPC util method (Sjors)
- bitcoin/bitcoin#15357 Don't ignore `-maxtxfee` when wallet is disabled (JBaczuk)
- bitcoin/bitcoin#15337 Fix for segfault if combinepsbt called with empty inputs (benthecarman)
- bitcoin/bitcoin#14918 RPCHelpMan: Check default values are given at compile-time (MarcoFalke)
- bitcoin/bitcoin#15383 mining: Omit uninitialized currentblockweight, currentblocktx (MarcoFalke)
- bitcoin/bitcoin#13932 Additional utility RPCs for PSBT (achow101)
- bitcoin/bitcoin#15401 Actually throw help when passed invalid number of params (MarcoFalke)
- bitcoin/bitcoin#15471 rpc/gui: Remove 'Unknown block versions being mined' warning (laanwj)
- bitcoin/bitcoin#15497 Consistent range arguments in scantxoutset/importmulti/deriveaddresses (sipa)
- bitcoin/bitcoin#15510 deriveaddresses: add range to CRPCConvertParam (Sjors)
- bitcoin/bitcoin#15582 Fix overflow bug in analyzepsbt fee: CAmount instead of int (sipa)
- bitcoin/bitcoin#13424 Consistently validate txid / blockhash length and encoding in rpc calls (Empact)
- bitcoin/bitcoin#15750 Remove the addresses field from the getaddressinfo return object (jnewbery)
- bitcoin/bitcoin#15991 Bugfix: fix pruneblockchain returned prune height (jonasschnelli)
- bitcoin/bitcoin#15899 Document iswitness flag and fix bug in converttopsbt (MarcoFalke)
- bitcoin/bitcoin#16026 Ensure that uncompressed public keys in a multisig always returns a legacy address (achow101)
- bitcoin/bitcoin#14039 Disallow extended encoding for non-witness transactions (sipa)
- bitcoin/bitcoin#16210 add 2nd arg to signrawtransactionwithkey examples (dooglus)
- bitcoin/bitcoin#16250 signrawtransactionwithkey: report error when missing redeemScript/witnessScript (ajtowns)

### GUI
- bitcoin/bitcoin#13634 Compile `boost::signals2` only once (MarcoFalke)
- bitcoin/bitcoin#13248 Make proxy icon from statusbar clickable (mess110)
- bitcoin/bitcoin#12818 TransactionView: highlight replacement tx after fee bump (Sjors)
- bitcoin/bitcoin#13529 Use new Qt5 connect syntax (promag)
- bitcoin/bitcoin#14162 Also log and print messages or questions like bitcoind (MarcoFalke)
- bitcoin/bitcoin#14385 Avoid system harfbuzz and bz2 (theuni)
- bitcoin/bitcoin#14450 Fix QCompleter popup regression (hebasto)
- bitcoin/bitcoin#14177 Set C locale for amountWidget (hebasto)
- bitcoin/bitcoin#14374 Add `Blocksdir` to Debug window (hebasto)
- bitcoin/bitcoin#14554 Remove unused `adjustedTime` parameter (hebasto)
- bitcoin/bitcoin#14228 Enable system tray icon by default if available (hebasto)
- bitcoin/bitcoin#14608 Remove the "Pay only required fee…" checkbox (hebasto)
- bitcoin/bitcoin#14521 qt, docs: Fix `bitcoin-qt -version` output formatting (hebasto)
- bitcoin/bitcoin#13966 When private key is disabled, only show watch-only balance (ken2812221)
- bitcoin/bitcoin#14828 Remove hidden columns in coin control dialog (promag)
- bitcoin/bitcoin#14783 Fix `boost::signals2::no_slots_error` in early calls to InitWarning (promag)
- bitcoin/bitcoin#14854 Cleanup SplashScreen class (hebasto)
- bitcoin/bitcoin#14801 Use window() instead of obsolete topLevelWidget() (hebasto)
- bitcoin/bitcoin#14573 Add Window menu (promag)
- bitcoin/bitcoin#14979 Restore < Qt5.6 compatibility for addAction (jonasschnelli)
- bitcoin/bitcoin#14975 Refactoring with QString::toNSString() (hebasto)
- bitcoin/bitcoin#15000 Fix broken notificator on GNOME (hebasto)
- bitcoin/bitcoin#14375 Correct misleading "overridden options" label (hebasto)
- bitcoin/bitcoin#15007 Notificator class refactoring (hebasto)
- bitcoin/bitcoin#14784 Use `WalletModel*` instead of the wallet name as map key (promag)
- bitcoin/bitcoin#11625 Add BitcoinApplication & RPCConsole tests (ryanofsky)
- bitcoin/bitcoin#14517 Fix start with the `-min` option (hebasto)
- bitcoin/bitcoin#13216 implements concept for different disk sizes on intro (marcoagner)
- bitcoin/bitcoin#15114 Replace remaining 0 with nullptr (Empact)
- bitcoin/bitcoin#14594 Fix minimized window bug on Linux (hebasto)
- bitcoin/bitcoin#14556 Fix confirmed transaction labeled "open" (bitcoin/bitcoin#13299) (hebasto)
- bitcoin/bitcoin#15149 Show current wallet name in window title (promag)
- bitcoin/bitcoin#15136 "Peers" tab overhaul (hebasto)
- bitcoin/bitcoin#14250 Remove redundant stopThread() and stopExecutor() signals (hebasto)
- bitcoin/bitcoin#15040 Add workaround for QProgressDialog bug on macOS (hebasto)
- bitcoin/bitcoin#15101 Add WalletController (promag)
- bitcoin/bitcoin#15178 Improve "help-console" message (hebasto)
- bitcoin/bitcoin#15210 Fix window title update (promag)
- bitcoin/bitcoin#15167 Fix wallet selector size adjustment (hebasto)
- bitcoin/bitcoin#15208 Remove macOS launch-at-startup when compiled with > macOS 10.11, fix memory mismanagement (jonasschnelli)
- bitcoin/bitcoin#15163 Correct units for "-dbcache" and "-prune" (hebasto)
- bitcoin/bitcoin#15225 Change the receive button to respond to keypool state changing (achow101)
- bitcoin/bitcoin#15280 Fix shutdown order (promag)
- bitcoin/bitcoin#15203 Fix issue #9683 "gui, wallet: random abort (segmentation fault) (dooglus)
- bitcoin/bitcoin#15091 Fix model overlay header sync (jonasschnelli)
- bitcoin/bitcoin#15153 Add Open Wallet menu (promag)
- bitcoin/bitcoin#15183 Fix `m_assumed_blockchain_size` variable value (marcoagner)
- bitcoin/bitcoin#15063 If BIP70 is disabled, attempt to fall back to BIP21 parsing (luke-jr)
- bitcoin/bitcoin#15195 Add Close Wallet action (promag)
- bitcoin/bitcoin#15462 Fix async open wallet call order (promag)
- bitcoin/bitcoin#15801 Bugfix: GUI: Options: Initialise prune setting range before loading current value, and remove upper bound limit (luke-jr)
- bitcoin/bitcoin#16044 fix the bug of OPEN CONFIGURATION FILE on Mac (shannon1916)
- bitcoin/bitcoin#15957 Show "No wallets available" in open menu instead of nothing (meshcollider)
- bitcoin/bitcoin#16118 Enable open wallet menu on setWalletController (promag)
- bitcoin/bitcoin#16135 Set progressDialog to nullptr (promag)
- bitcoin/bitcoin#16231 Fix open wallet menu initialization order (promag) 
- bitcoin/bitcoin#16254 Set `AA_EnableHighDpiScaling` attribute early (hebasto) 
- bitcoin/bitcoin#16122 Enable console line edit on setClientModel (promag) 
- bitcoin/bitcoin#16348 Assert QMetaObject::invokeMethod result (promag)

### Build system
- bitcoin/bitcoin#13955 gitian: Bump descriptors for (0.)18 (fanquake)
- bitcoin/bitcoin#13899 Enable -Wredundant-decls where available. Remove redundant redeclarations (practicalswift)
- bitcoin/bitcoin#13665 Add RISC-V support to gitian (ken2812221)
- bitcoin/bitcoin#14062 Generate MSVC project files via python script (ken2812221)
- bitcoin/bitcoin#14037 Add README.md to linux release tarballs (hebasto)
- bitcoin/bitcoin#14183 Remove unused Qt 4 dependencies (ken2812221)
- bitcoin/bitcoin#14127 Avoid getifaddrs when unavailable (greenaddress)
- bitcoin/bitcoin#14184 Scripts and tools: increased timeout downloading (cisba)
- bitcoin/bitcoin#14204 Move `interfaces/*` to `libbitcoin_server` (laanwj)
- bitcoin/bitcoin#14208 Actually remove `ENABLE_WALLET` (jnewbery)
- bitcoin/bitcoin#14212 Remove libssl from LDADD unless GUI (MarcoFalke)
- bitcoin/bitcoin#13578 Upgrade zeromq to 4.2.5 and avoid deprecated zeromq API functions (mruddy)
- bitcoin/bitcoin#14281 lcov: filter /usr/lib/ from coverage reports (MarcoFalke)
- bitcoin/bitcoin#14325 gitian: Use versioned unsigned tarballs instead of generically named ones (achow101)
- bitcoin/bitcoin#14253 During 'make clean', remove some files that are currently missed (murrayn)
- bitcoin/bitcoin#14455 Unbreak `make clean` (jamesob)
- bitcoin/bitcoin#14495 Warn (don't fail!) on spelling errors (practicalswift)
- bitcoin/bitcoin#14496 Pin to specific versions of Python packages we install from PyPI in Travis (practicalswift)
- bitcoin/bitcoin#14568 Fix Qt link order for Windows build (ken2812221)
- bitcoin/bitcoin#14252 Run functional tests and benchmarks under the undefined behaviour sanitizer (UBSan) (practicalswift)
- bitcoin/bitcoin#14612 Include full version number in released file names (achow101)
- bitcoin/bitcoin#14840 Remove duplicate libconsensus linking in test make (AmirAbrams)
- bitcoin/bitcoin#14564 Adjust configure so that only BIP70 is disabled when protobuf is missing instead of the GUI (jameshilliard)
- bitcoin/bitcoin#14883 Add `--retry 5` to curl opts in `install_db4.sh` (qubenix)
- bitcoin/bitcoin#14701 Add `CLIENT_VERSION_BUILD` to CFBundleGetInfoString (fanquake)
- bitcoin/bitcoin#14849 Qt 5.9.7 (fanquake)
- bitcoin/bitcoin#15020 Add names to Travis jobs (gkrizek)
- bitcoin/bitcoin#15047 Allow to configure --with-sanitizers=fuzzer (MarcoFalke)
- bitcoin/bitcoin#15154 Configure: bitcoin-tx doesn't need libevent, so don't pull it in (luke-jr)
- bitcoin/bitcoin#15175 Drop macports support (Empact)
- bitcoin/bitcoin#15308 Restore compatibility with older boost (Empact)
- bitcoin/bitcoin#15407 msvc: Fix silent merge conflict between #13926 and #14372 part II (ken2812221)
- bitcoin/bitcoin#15388 Makefile.am: add rule for src/bitcoin-wallet (Sjors)
- bitcoin/bitcoin#15393 Bump minimum Qt version to 5.5.1 (Sjors)
- bitcoin/bitcoin#15285 Prefer Python 3.4 even if newer versions are present on the system (Sjors)
- bitcoin/bitcoin#15398 msvc: Add rapidcheck property tests (ken2812221)
- bitcoin/bitcoin#15431 msvc: scripted-diff: Remove NDEBUG pre-define in project file (ken2812221)
- bitcoin/bitcoin#15549 gitian: Improve error handling (laanwj)
- bitcoin/bitcoin#15548 use full version string in setup.exe (MarcoFalke)
- bitcoin/bitcoin#11526 Visual Studio build configuration for Bitcoin Core (sipsorcery)
- bitcoin/bitcoin#15110 build\_msvc: Fix the build problem in `libbitcoin_server` (Mr-Leshiy)
- bitcoin/bitcoin#14372 msvc: build secp256k1 and leveldb locally (ken2812221)
- bitcoin/bitcoin#15325 msvc: Fix silent merge conflict between #13926 and #14372 (ken2812221)
- bitcoin/bitcoin#15391 Add compile time verification of assumptions we're currently making implicitly/tacitly (practicalswift)
- bitcoin/bitcoin#15503 msvc: Use a single file to specify the include path (ken2812221)
- bitcoin/bitcoin#13765 contrib: Add gitian build support for github pull request (ken2812221)
- bitcoin/bitcoin#15809 gitignore: plist and dat (jamesob)
- bitcoin/bitcoin#15985 Add test for GCC bug 90348 (sipa)
- bitcoin/bitcoin#15947 Install bitcoin-wallet manpage (domob1812)
- bitcoin/bitcoin#15983 build with -fstack-reuse=none (MarcoFalke)

### Tests and QA
- bitcoin/bitcoin#15405 appveyor: Clean cache when build configuration changes (Sjors)
- bitcoin/bitcoin#13953 Fix deprecation in bitcoin-util-test.py (isghe)
- bitcoin/bitcoin#13963 Replace usage of tostring() with tobytes() (dongcarl)
- bitcoin/bitcoin#13964 ci: Add appveyor ci (ken2812221)
- bitcoin/bitcoin#13997 appveyor: fetch the latest port data (ken2812221)
- bitcoin/bitcoin#13707 Add usage note to check-rpc-mappings.py (masonicboom)
- bitcoin/bitcoin#14036 travis: Run unit tests --with-sanitizers=undefined (MarcoFalke)
- bitcoin/bitcoin#13861 Add testing of `value_ret` for SelectCoinsBnB (Empact)
- bitcoin/bitcoin#13863 travis: Move script sections to files in `.travis/` subject to shellcheck (scravy)
- bitcoin/bitcoin#14081 travis: Fix missing differentiation between unit and functional tests (scravy)
- bitcoin/bitcoin#14042 travis: Add cxxflags=-wno-psabi at arm job (ken2812221)
- bitcoin/bitcoin#14051 Make `combine_logs.py` handle multi-line logs (jnewbery)
- bitcoin/bitcoin#14093 Fix accidental trunction from int to bool (practicalswift)
- bitcoin/bitcoin#14108 Add missing locking annotations and locks (`g_cs_orphans`) (practicalswift)
- bitcoin/bitcoin#14088 Don't assert(…) with side effects (practicalswift)
- bitcoin/bitcoin#14086 appveyor: Use clcache to speed up build (ken2812221)
- bitcoin/bitcoin#13954 Warn (don't fail!) on spelling errors. Fix typos reported by codespell (practicalswift)
- bitcoin/bitcoin#12775 Integration of property based testing into Bitcoin Core (Christewart)
- bitcoin/bitcoin#14119 Read reject reasons from debug log, not P2P messages (MarcoFalke)
- bitcoin/bitcoin#14189 Fix silent merge conflict in `wallet_importmulti` (MarcoFalke)
- bitcoin/bitcoin#13419 Speed up `knapsack_solver_test` by not recreating wallet 100 times (lucash-dev)
- bitcoin/bitcoin#14199 Remove redundant BIP174 test from `rpc_psbt.json` (araspitzu)
- bitcoin/bitcoin#14179 Fixups to "Run all tests even if wallet is not compiled" (MarcoFalke)
- bitcoin/bitcoin#14225 Reorder tests and move most of extended tests up to normal tests (ken2812221)
- bitcoin/bitcoin#14236 `generate` --> `generatetoaddress` change to allow tests run without wallet (sanket1729)
- bitcoin/bitcoin#14287 Use MakeUnique to construct objects owned by `unique_ptrs` (practicalswift)
- bitcoin/bitcoin#14007 Run functional test on Windows and enable it on Appveyor (ken2812221)
- bitcoin/bitcoin#14275 Write the notification message to different files to avoid race condition in `feature_notifications.py` (ken2812221)
- bitcoin/bitcoin#14306 appveyor: Move AppVeyor YAML to dot-file-style YAML (MitchellCash)
- bitcoin/bitcoin#14305 Enforce critical class instance attributes in functional tests, fix segwit test specificity (JustinTArthur)
- bitcoin/bitcoin#12246 Bugfix: Only run bitcoin-tx tests when bitcoin-tx is enabled (luke-jr)
- bitcoin/bitcoin#14316 Exclude all tests with difference parameters in `--exclude` list (ken2812221)
- bitcoin/bitcoin#14381 Add missing call to `skip_if_no_cli()` (practicalswift)
- bitcoin/bitcoin#14389 travis: Set codespell version to avoid breakage (MarcoFalke)
- bitcoin/bitcoin#14398 Don't access out of bounds array index: array[sizeof(array)] (Empact)
- bitcoin/bitcoin#14419 Remove `rpc_zmq.py` (jnewbery)
- bitcoin/bitcoin#14241 appveyor: Script improvement (ken2812221)
- bitcoin/bitcoin#14413 Allow closed RPC handler in `assert_start_raises_init_error` (ken2812221)
- bitcoin/bitcoin#14324 Run more tests with wallet disabled (MarcoFalke)
- bitcoin/bitcoin#13649 Allow arguments to be forwarded to flake8 in lint-python.sh (jamesob)
- bitcoin/bitcoin#14465 Stop node before removing the notification file (ken2812221)
- bitcoin/bitcoin#14460 Improve 'CAmount' tests (hebasto)
- bitcoin/bitcoin#14456 forward timeouts properly in `send_blocks_and_test` (jamesob)
- bitcoin/bitcoin#14527 Revert "Make qt wallet test compatible with qt4" (MarcoFalke)
- bitcoin/bitcoin#14504 Show the progress of functional tests (isghe)
- bitcoin/bitcoin#14559 appveyor: Enable multiwallet tests (ken2812221)
- bitcoin/bitcoin#13515 travis: Enable qt for all jobs (ken2812221)
- bitcoin/bitcoin#14571 Test that nodes respond to `getdata` with `notfound` (MarcoFalke)
- bitcoin/bitcoin#14569 Print dots by default in functional tests (ken2812221)
- bitcoin/bitcoin#14631 Move deterministic address import to `setup_nodes` (jnewbery)
- bitcoin/bitcoin#14630 test: Remove travis specific code (MarcoFalke)
- bitcoin/bitcoin#14528 travis: Compile once on xenial (MarcoFalke)
- bitcoin/bitcoin#14092 Dry run `bench_bitcoin` as part `make check` to allow for quick identification of assertion/sanitizer failures in benchmarking code (practicalswift)
- bitcoin/bitcoin#14664 `example_test.py`: fixup coinbase height argument, derive number clearly (instagibbs)
- bitcoin/bitcoin#14522 Add invalid P2P message tests (jamesob)
- bitcoin/bitcoin#14619 Fix value display name in `test_runner` help text (merland)
- bitcoin/bitcoin#14672 Send fewer spam messages in `p2p_invalid_messages` (jamesob)
- bitcoin/bitcoin#14673 travis: Fail the ubsan travis build in case of newly introduced ubsan errors (practicalswift)
- bitcoin/bitcoin#14665 appveyor: Script improvement part II (ken2812221)
- bitcoin/bitcoin#14365 Add Python dead code linter (vulture) to Travis (practicalswift)
- bitcoin/bitcoin#14693 `test_node`: `get_mem_rss` fixups (MarcoFalke)
- bitcoin/bitcoin#14714 util.h: explicitly include required QString header (1Il1)
- bitcoin/bitcoin#14705 travis: Avoid timeout on verify-commits check (MarcoFalke)
- bitcoin/bitcoin#14770 travis: Do not specify sudo in `.travis` (scravy)
- bitcoin/bitcoin#14719 Check specific reject reasons in `feature_block` (MarcoFalke)
- bitcoin/bitcoin#14771 Add `BOOST_REQUIRE` to getters returning optional (MarcoFalke)
- bitcoin/bitcoin#14777 Add regtest for JSON-RPC batch calls (domob1812)
- bitcoin/bitcoin#14764 travis: Run thread sanitizer on unit tests (MarcoFalke)
- bitcoin/bitcoin#14400 Add Benchmark to test input de-duplication worst case (JeremyRubin)
- bitcoin/bitcoin#14812 Fix `p2p_invalid_messages` on macOS (jamesob)
- bitcoin/bitcoin#14813 Add `wallet_encryption` error tests (MarcoFalke)
- bitcoin/bitcoin#14820 Fix `descriptor_tests` not checking ToString output of public descriptors (ryanofsky)
- bitcoin/bitcoin#14794 Add AddressSanitizer (ASan) Travis build (practicalswift)
- bitcoin/bitcoin#14819 Bugfix: `test/functional/mempool_accept`: Ensure oversize transaction is actually oversize (luke-jr)
- bitcoin/bitcoin#14822 bench: Destroy wallet txs instead of leaking their memory (MarcoFalke)
- bitcoin/bitcoin#14683 Better `combine_logs.py` behavior (jamesob)
- bitcoin/bitcoin#14231 travis: Save cache even when build or test fail (ken2812221)
- bitcoin/bitcoin#14816 Add CScriptNum decode python implementation in functional suite (instagibbs)
- bitcoin/bitcoin#14861 Modify `rpc_bind` to conform to #14532 behaviour (dongcarl)
- bitcoin/bitcoin#14864 Run scripted-diff in subshell (dongcarl)
- bitcoin/bitcoin#14795 Allow `test_runner` command line to receive parameters for each test (marcoagner)
- bitcoin/bitcoin#14788 Possible fix the permission error when the tests open the cookie file (ken2812221)
- bitcoin/bitcoin#14857 `wallet_keypool_topup.py`: Test for all keypool address types (instagibbs)
- bitcoin/bitcoin#14886 Refactor importmulti tests (jnewbery)
- bitcoin/bitcoin#14908 Removed implicit CTransaction constructor calls from tests and benchmarks (lucash-dev)
- bitcoin/bitcoin#14903 Handle ImportError explicitly, improve comparisons against None (daniel-s-ingram)
- bitcoin/bitcoin#14884 travis: Enforce python 3.4 support through linter (Sjors)
- bitcoin/bitcoin#14940 Add test for truncated pushdata script (MarcoFalke)
- bitcoin/bitcoin#14926 consensus: Check that final transactions are valid (MarcoFalke)
- bitcoin/bitcoin#14937 travis: Fix travis would always be green even if it fail (ken2812221)
- bitcoin/bitcoin#14953 Make `g_insecure_rand_ctx` `thread_local` (MarcoFalke)
- bitcoin/bitcoin#14931 mempool: Verify prioritization is dumped correctly (MarcoFalke)
- bitcoin/bitcoin#14935 Test for expected return values when calling functions returning a success code (practicalswift)
- bitcoin/bitcoin#14969 Fix `cuckoocache_tests` TSAN failure introduced in 14935 (practicalswift)
- bitcoin/bitcoin#14964 Fix race in `mempool_accept` (MarcoFalke)
- bitcoin/bitcoin#14829 travis: Enable functional tests in the threadsanitizer (tsan) build job (practicalswift)
- bitcoin/bitcoin#14985 Remove `thread_local` from `test_bitcoin` (MarcoFalke)
- bitcoin/bitcoin#15005 Bump timeout to run tests in travis thread sanitizer (MarcoFalke)
- bitcoin/bitcoin#15013 Avoid race in `p2p_timeouts` (MarcoFalke)
- bitcoin/bitcoin#14960 lint/format-strings: Correctly exclude escaped percent symbols (luke-jr)
- bitcoin/bitcoin#14930 pruning: Check that verifychain can be called when pruned (MarcoFalke)
- bitcoin/bitcoin#15022 Upgrade Travis OS to Xenial (gkrizek)
- bitcoin/bitcoin#14738 Fix running `wallet_listtransactions.py` individually through `test_runner.py` (kristapsk)
- bitcoin/bitcoin#15026 Rename `rpc_timewait` to `rpc_timeout` (MarcoFalke)
- bitcoin/bitcoin#15069 Fix `rpc_net.py` `pong` race condition (Empact)
- bitcoin/bitcoin#14790 Allow running `rpc_bind.py` --nonloopback test without IPv6 (kristapsk)
- bitcoin/bitcoin#14457 add invalid tx templates for use in functional tests (jamesob)
- bitcoin/bitcoin#14855 Correct ineffectual WithOrVersion from `transactions_tests` (Empact)
- bitcoin/bitcoin#15099 Use `std::vector` API for construction of test data (domob1812)
- bitcoin/bitcoin#15102 Run `invalid_txs.InputMissing` test in `feature_block` (MarcoFalke)
- bitcoin/bitcoin#15059 Add basic test for BIP34 (MarcoFalke)
- bitcoin/bitcoin#15108 Tidy up `wallet_importmulti.py` (amitiuttarwar)
- bitcoin/bitcoin#15164 Ignore shellcheck warning SC2236 (promag)
- bitcoin/bitcoin#15170 refactor/lint: Add ignored shellcheck suggestions to an array (koalaman)
- bitcoin/bitcoin#14958 Remove race between connecting and shutdown on separate connections (promag)
- bitcoin/bitcoin#15166 Pin shellcheck version (practicalswift)
- bitcoin/bitcoin#15196 Update all `subprocess.check_output` functions to be Python 3.4 compatible (gkrizek)
- bitcoin/bitcoin#15043 Build fuzz targets into seperate executables (MarcoFalke)
- bitcoin/bitcoin#15276 travis: Compile once on trusty (MarcoFalke)
- bitcoin/bitcoin#15246 Add tests for invalid message headers (MarcoFalke)
- bitcoin/bitcoin#15301 When testing with --usecli, unify RPC arg to cli arg conversion and handle dicts and lists (achow101)
- bitcoin/bitcoin#15247 Use wallet to retrieve raw transactions (MarcoFalke)
- bitcoin/bitcoin#15303 travis: Remove unused `functional_tests_config` (MarcoFalke)
- bitcoin/bitcoin#15330 Fix race in `p2p_invalid_messages` (MarcoFalke)
- bitcoin/bitcoin#15324 Make bloom tests deterministic (MarcoFalke)
- bitcoin/bitcoin#15328 travis: Revert "run extended tests once daily" (MarcoFalke)
- bitcoin/bitcoin#15327 Make test `updatecoins_simulation_test` deterministic (practicalswift)
- bitcoin/bitcoin#14519 add utility to easily profile node performance with perf (jamesob)
- bitcoin/bitcoin#15349 travis: Only exit early if compilation took longer than 30 min (MarcoFalke)
- bitcoin/bitcoin#15350 Drop RPC connection if --usecli (promag)
- bitcoin/bitcoin#15370 test: Remove unused --force option (MarcoFalke)
- bitcoin/bitcoin#14543 minor `p2p_sendheaders` fix of height in coinbase (instagibbs)
- bitcoin/bitcoin#13787 Test for Windows encoding issue (ken2812221)
- bitcoin/bitcoin#15378 Added missing tests for RPC wallet errors (benthecarman)
- bitcoin/bitcoin#15238 remove some magic mining constants in functional tests (instagibbs)
- bitcoin/bitcoin#15411 travis: Combine --disable-bip70 into existing job (MarcoFalke)
- bitcoin/bitcoin#15295 fuzz: Add `test/fuzz/test_runner.py` and run it in travis (MarcoFalke)
- bitcoin/bitcoin#15413 Add missing `cs_main` locks required when accessing pcoinsdbview, pcoinsTip or pblocktree (practicalswift)
- bitcoin/bitcoin#15399 fuzz: Script validation flags (MarcoFalke)
- bitcoin/bitcoin#15410 txindex: interrupt threadGroup before calling destructor (MarcoFalke)
- bitcoin/bitcoin#15397 Remove manual byte editing in `wallet_tx_clone` func test (instagibbs)
- bitcoin/bitcoin#15415 functional: allow custom cwd, use tmpdir as default (Sjors)
- bitcoin/bitcoin#15404 Remove `-txindex` to start nodes (amitiuttarwar)
- bitcoin/bitcoin#15439 remove `byte.hex()` to keep compatibility (AkioNak)
- bitcoin/bitcoin#15419 Always refresh cache to be out of ibd (MarcoFalke)
- bitcoin/bitcoin#15507 Bump timeout on tests that timeout on windows (MarcoFalke)
- bitcoin/bitcoin#15506 appveyor: fix cache issue and reduce dependencies build time (ken2812221)
- bitcoin/bitcoin#15485 add `rpc_misc.py`, mv test getmemoryinfo, add test mallocinfo (adamjonas)
- bitcoin/bitcoin#15321 Add `cs_main` lock annotations for mapBlockIndex (MarcoFalke)
- bitcoin/bitcoin#14128 lint: Make sure we read the command line inputs using UTF-8 decoding in python (ken2812221)
- bitcoin/bitcoin#14115 lint: Make all linters work under the default macos dev environment (build-osx.md) (practicalswift)
- bitcoin/bitcoin#15219 lint: Enable python linters via an array (Empact)
- bitcoin/bitcoin#15826 Pure python EC (sipa)
- bitcoin/bitcoin#15893 Add test for superfluous witness record in deserialization (instagibbs)
- bitcoin/bitcoin#14818 Bugfix: test/functional/rpc_psbt: Remove check for specific error message that depends on uncertain assumptions (luke-jr)
- bitcoin/bitcoin#15831 Add test that addmultisigaddress fails for watchonly addresses (MarcoFalke)

### Platform support
- bitcoin/bitcoin#13866 utils: Use `_wfopen` and `_wfreopen` on windows (ken2812221)
- bitcoin/bitcoin#13886 utils: Run commands using UTF-8 string on windows (ken2812221)
- bitcoin/bitcoin#14192 utils: Convert `fs::filesystem_error` messages from local multibyte to UTF-8 on windows (ken2812221)
- bitcoin/bitcoin#13877 utils: Make fs::path::string() always return UTF-8 string on windows (ken2812221)
- bitcoin/bitcoin#13883 utils: Convert windows args to UTF-8 string (ken2812221)
- bitcoin/bitcoin#13878 utils: Add fstream wrapper to allow to pass unicode filename on windows (ken2812221)
- bitcoin/bitcoin#14426 utils: Fix broken windows filelock (ken2812221)
- bitcoin/bitcoin#14686 Fix windows build error if `--disable-bip70` (ken2812221)
- bitcoin/bitcoin#14922 windows: Set `_WIN32_WINNT` to 0x0601 (Windows 7) (ken2812221)
- bitcoin/bitcoin#13888 Call unicode API on Windows (ken2812221)
- bitcoin/bitcoin#15468 Use `fsbridge::ifstream` to fix Windows path issue (ken2812221)
- bitcoin/bitcoin#13734 Drop `boost::scoped_array` and use `wchar_t` API explicitly on Windows (ken2812221)
- bitcoin/bitcoin#13884 Enable bdb unicode support for Windows (ken2812221)

### Miscellaneous
- bitcoin/bitcoin#13935 contrib: Adjust output to current test format (AkioNak)
- bitcoin/bitcoin#14097 validation: Log FormatStateMessage on ConnectBlock error in ConnectTip (MarcoFalke)
- bitcoin/bitcoin#13724 contrib: Support ARM and RISC-V symbol check (ken2812221)
- bitcoin/bitcoin#13159 Don't close old debug log file handle prematurely when trying to re-open (on SIGHUP) (practicalswift)
- bitcoin/bitcoin#14186 bitcoin-cli: don't translate command line options (HashUnlimited)
- bitcoin/bitcoin#14057 logging: Only log `using config file path_to_bitcoin.conf` message on startup if conf file exists (leishman)
- bitcoin/bitcoin#14164 Update univalue subtree (MarcoFalke)
- bitcoin/bitcoin#14272 init: Remove deprecated args from hidden args (MarcoFalke)
- bitcoin/bitcoin#14494 Error if # is used in rpcpassword in conf (MeshCollider)
- bitcoin/bitcoin#14742 Properly generate salt in rpcauth.py (dongcarl)
- bitcoin/bitcoin#14708 Warn unrecognised sections in the config file (AkioNak)
- bitcoin/bitcoin#14756 Improve rpcauth.py by using argparse and getpass modules (promag)
- bitcoin/bitcoin#14785 scripts: Fix detection of copyright holders (cornelius)
- bitcoin/bitcoin#14831 scripts: Use `#!/usr/bin/env bash` instead of `#!/bin/bash` (vim88)
- bitcoin/bitcoin#14869 Scripts: Add trusted key for samuel dobson (laanwj)
- bitcoin/bitcoin#14809 Tools: improve verify-commits.py script (jlopp)
- bitcoin/bitcoin#14624 Some simple improvements to the RNG code (sipa)
- bitcoin/bitcoin#14947 scripts: Remove python 2 import workarounds (practicalswift)
- bitcoin/bitcoin#15087 Error if rpcpassword contains hash in conf sections (MeshCollider)
- bitcoin/bitcoin#14433 Add checksum in gitian build scripts for ossl (TheCharlatan)
- bitcoin/bitcoin#15165 contrib: Allow use of github api authentication in github-merge (laanwj)
- bitcoin/bitcoin#14409 utils and libraries: Make 'blocksdir' always net specific (hebasto)
- bitcoin/bitcoin#14839 threads: Fix unitialized members in `sched_param` (fanquake)
- bitcoin/bitcoin#14955 Switch all RNG code to the built-in PRNG (sipa)
- bitcoin/bitcoin#15258 Scripts and tools: Fix `devtools/copyright_header.py` to always honor exclusions (Empact)
- bitcoin/bitcoin#12255 Update bitcoin.service to conform to init.md (dongcarl)
- bitcoin/bitcoin#15266 memory: Construct globals on first use (MarcoFalke)
- bitcoin/bitcoin#15347 Fix build after pr 15266 merged (hebasto)
- bitcoin/bitcoin#15351 Update linearize-hashes.py (OverlordQ)
- bitcoin/bitcoin#15358 util: Add setuphelpoptions() (MarcoFalke)
- bitcoin/bitcoin#15216 Scripts and tools: Replace script name with a special parameter (hebasto)
- bitcoin/bitcoin#15250 Use RdSeed when available, and reduce RdRand load (sipa)
- bitcoin/bitcoin#15278 Improve PID file error handling (hebasto)
- bitcoin/bitcoin#15270 Pull leveldb subtree (MarcoFalke)
- bitcoin/bitcoin#15456 Enable PID file creation on WIN (riordant)
- bitcoin/bitcoin#12783 macOS: disable AppNap during sync (krab)
- bitcoin/bitcoin#13910 Log progress while verifying blocks at level 4 (domob1812)
- bitcoin/bitcoin#15124 Fail AppInitMain if either disk space check fails (Empact)
- bitcoin/bitcoin#15117 Fix invalid memory write in case of failing mmap(…) in PosixLockedPageAllocator::AllocateLocked (practicalswift)
- bitcoin/bitcoin#14357 streams: Fix broken `streams_vector_reader` test. Remove unused `seek(size_t)`
- bitcoin/bitcoin#11640 Make `LOCK`, `LOCK2`, `TRY_LOCK` work with CWaitableCriticalSection (ryanofsky)
- bitcoin/bitcoin#14074 Use `std::unordered_set` instead of `set` in blockfilter interface (jimpo)
- bitcoin/bitcoin#15275 Add gitian PGP key for hebasto (hebasto)
- bitcoin/bitcoin#16095 Catch by reference not value in wallettool (kristapsk)
- bitcoin/bitcoin#16205 Replace fprintf with tfm::format (MarcoFalke)

### Documentation
- bitcoin/bitcoin#14120 Notes about control port and read access to cookie (JBaczuk)
- bitcoin/bitcoin#14135 correct GetDifficulty doc after #13288 (fanquake)
- bitcoin/bitcoin#14013 Add new regtest ports in man following #10825 ports reattributions (ariard)
- bitcoin/bitcoin#14149 Remove misleading checkpoints comment in CMainParams (MarcoFalke)
- bitcoin/bitcoin#14153 Add disable-wallet section to OSX build instructions, update line in Unix instructions (bitstein)
- bitcoin/bitcoin#13662 Explain when reindex-chainstate can be used instead of reindex (Sjors)
- bitcoin/bitcoin#14207 `-help-debug` implies `-help` (laanwj)
- bitcoin/bitcoin#14213 Fix reference to lint-locale-dependence.sh (hebasto)
- bitcoin/bitcoin#14206 Document `-checklevel` levels (laanwj)
- bitcoin/bitcoin#14217 Add GitHub PR template (MarcoFalke)
- bitcoin/bitcoin#14331 doxygen: Fix member comments (MarcoFalke)
- bitcoin/bitcoin#14264 Split depends installation instructions per arch (MarcoFalke)
- bitcoin/bitcoin#14393 Add missing apt-get install (poiuty)
- bitcoin/bitcoin#14428 Fix macOS files description in qt/README.md (hebasto)
- bitcoin/bitcoin#14390 release process: RPC documentation (karel-3d)
- bitcoin/bitcoin#14472 getblocktemplate: use SegWit in example (Sjors)
- bitcoin/bitcoin#14497 Add doc/bitcoin-conf.md (hebasto)
- bitcoin/bitcoin#14526 Document lint tests (fanquake)
- bitcoin/bitcoin#14511 Remove explicit storage requirement from README.md (merland)
- bitcoin/bitcoin#14600 Clarify commit message guidelines (merland)
- bitcoin/bitcoin#14617 FreeBSD: Document Python 3 requirement for 'gmake check' (murrayn)
- bitcoin/bitcoin#14592 Add external interface consistency guarantees (MarcoFalke)
- bitcoin/bitcoin#14625 Make clear function argument case in dev notes (dongcarl)
- bitcoin/bitcoin#14515 Update OpenBSD build guide for 6.4 (fanquake)
- bitcoin/bitcoin#14436 Add comment explaining recentRejects-DoS behavior (jamesob)
- bitcoin/bitcoin#14684 conf: Remove deprecated options from docs, Other cleanup (MarcoFalke)
- bitcoin/bitcoin#14731 Improve scripted-diff developer docs (dongcarl)
- bitcoin/bitcoin#14778 A few minor formatting fixes and clarifications to descriptors.md (jnewbery)
- bitcoin/bitcoin#14448 Clarify rpcwallet flag url change (JBaczuk)
- bitcoin/bitcoin#14808 Clarify RPC rawtransaction documentation (jlopp)
- bitcoin/bitcoin#14804 Less confusing documentation for `torpassword` (fanquake)
- bitcoin/bitcoin#14848 Fix broken Gmane URL in security-check.py (cyounkins-bot)
- bitcoin/bitcoin#14882 developer-notes.md: Point out that UniValue deviates from upstream (Sjors)
- bitcoin/bitcoin#14909 Update minimum required Qt (fanquake)
- bitcoin/bitcoin#14914 Add nice table to files.md (emilengler)
- bitcoin/bitcoin#14741 Indicate `-rpcauth` option password hashing alg (dongcarl)
- bitcoin/bitcoin#14950 Add NSIS setup/install steps to windows docs (fanquake)
- bitcoin/bitcoin#13930 Better explain GetAncestor check for `m_failed_blocks` in AcceptBlockHeader (Sjors)
- bitcoin/bitcoin#14973 Improve Windows native build instructions (murrayn)
- bitcoin/bitcoin#15073 Botbot.me (IRC logs) not available anymore (anduck)
- bitcoin/bitcoin#15038 Get more info about GUI-related issue on Linux (hebasto)
- bitcoin/bitcoin#14832 Add more Doxygen information to Developer Notes (ch4ot1c)
- bitcoin/bitcoin#15128 Fix download link in doc/README.md (merland)
- bitcoin/bitcoin#15127 Clarifying testing instructions (benthecarman)
- bitcoin/bitcoin#15132 Add FreeBSD build notes link to doc/README.md (fanquake)
- bitcoin/bitcoin#15173 Explain what .python-version does (Sjors)
- bitcoin/bitcoin#15223 Add information about security to the JSON-RPC doc (harding)
- bitcoin/bitcoin#15249 Update python docs to reflect that wildcard imports are disallowed (Empact)
- bitcoin/bitcoin#15176 Get rid of badly named `doc/README_osx.md` (merland)
- bitcoin/bitcoin#15272 Correct logging return type and RPC example (fanquake)
- bitcoin/bitcoin#15244 Gdb attaching to process during tests has non-sudo solution (instagibbs)
- bitcoin/bitcoin#15332 Small updates to `getrawtransaction` description (amitiuttarwar)
- bitcoin/bitcoin#15354 Add missing `bitcoin-wallet` tool manpages (MarcoFalke)
- bitcoin/bitcoin#15343 netaddress: Make IPv4 loopback comment more descriptive (dongcarl)
- bitcoin/bitcoin#15353 Minor textual improvements in `translation_strings_policy.md` (merland)
- bitcoin/bitcoin#15426 importmulti: add missing description of keypool option (harding)
- bitcoin/bitcoin#15425 Add missing newline to listunspent help for witnessScript (harding)
- bitcoin/bitcoin#15348 Add separate productivity notes document (dongcarl)
- bitcoin/bitcoin#15416 Update FreeBSD build guide for 12.0 (fanquake)
- bitcoin/bitcoin#15222 Add info about factors that affect dependency list (merland)
- bitcoin/bitcoin#13676 Explain that mempool memory is added to `-dbcache` (Sjors)
- bitcoin/bitcoin#15273 Slight tweak to the verify-commits script directions (droark)
- bitcoin/bitcoin#15477 Remove misleading hint in getrawtransaction (MarcoFalke)
- bitcoin/bitcoin#15489 Update release process for snap package (MarcoFalke)
- bitcoin/bitcoin#15524 doc: Remove berkeleydb PPA from linux build instructions (MarcoFalke)
- bitcoin/bitcoin#15559 Correct `analyzepsbt` rpc doc (fanquake)
- bitcoin/bitcoin#15194 Add comment describing `fDisconnect` behavior (dongcarl)
- bitcoin/bitcoin#15754 getrpcinfo docs (benthecarman)
- bitcoin/bitcoin#15763 Update bips.md for 0.18.0 (sipa)
- bitcoin/bitcoin#15757 List new RPCs in psbt.md and descriptors.md (sipa)
- bitcoin/bitcoin#15765 correct bitcoinconsensus_version in shared-libraries.md (fanquake)
- bitcoin/bitcoin#15792 describe onlynet option in doc/tor.md (jonatack)
- bitcoin/bitcoin#15802 mention creating application support bitcoin folder on OSX (JimmyMow)
- bitcoin/bitcoin#15799 Clarify RPC versioning (MarcoFalke)
- bitcoin/bitcoin#15890 Remove text about txes always relayed from -whitelist (harding)

Credits
-------

Thanks to everyone who directly contributed to this release:

- 1Il1
- 251
- Aaron Clauson
- Adam Jonas
- Akio Nakamura
- Alexander Leishman
- Alexey Ivanov
- Alexey Poghilenkov
- Amir Abrams
- Amiti Uttarwar
- Andrew Chow
- andrewtoth
- Anthony Towns
- Antoine Le Calvez
- Antoine Riard
- Antti Majakivi
- araspitzu
- Arvid Norberg
- Ben Carman
- benthecarman
- Ben Woosley
- bitcoinhodler
- Carl Dong
- Chakib Benziane
- chris-belcher
- Chris Moore
- Chris Stewart
- Chun Kuan Lee
- Cornelius Schumacher
- Cory Fields
- Craig Younkins
- Cristian Mircea Messel
- Damian Mee
- Daniel Ingram
- Daniel Kraft
- David A. Harding
- DesWurstes
- dexX7
- Dimitri Deijs
- Dimitris Apostolou
- Douglas Roark
- DrahtBot
- Emanuele Cisbani
- Emil Engler
- Eric Scrivner
- fanquake
- fridokus
- Gal Buki
- Gleb Naumenko
- Glenn Willen
- Graham Krizek
- Gregory Maxwell
- Gregory Sanders
- gustavonalle
- Harry Moreno
- Hennadii Stepanov
- Isidoro Ghezzi
- Jack Mallers
- James Hilliard
- James O'Beirne
- Jameson Lopp
- Jeremy Rubin
- Jesse Cohen
- Jim Posen
- João Barbosa
- John Newbery
- Jonas Schnelli
- Jon Layton
- Jordan Baczuk
- Jorge Timón
- Julian Fleischer
- Justin Turner Arthur
- Karel Bílek
- Karl-Johan Alm
- Kaz Wesley
- ken2812221
- Kostiantyn Stepaniuk
- Kristaps Kaupe
- Lawrence Nahum
- Lenny Maiorani
- liuyujun
- lucash-dev
- luciana
- Luke Dashjr
- marcaiaf
- marcoagner
- MarcoFalke
- Mark Friedenbach
- Martin Erlandsson
- Marty Jones
- Mason Simon
- Michael Ford
- Michael Goldstein
- Michael Polzer
- Michele Federici
- Mitchell Cash
- mruddy
- Murray Nesbitt
- OverlordQ
- Patrick Strateman
- Pierre Rochard
- Pieter Wuille
- poiuty
- practicalswift
- priscoan
- qubenix
- riordant
- Russell Yanofsky
- Samuel Dobson
- sanket1729
- shannon1916
- Sjors Provoost
- Stephan Oeste
- Steven Roose
- Suhas Daftuar
- tecnovert
- TheCharlatan
- Tim Ruffing
- Vidar Holen
- vim88
- Walter
- whythat
- Wladimir J. van der Laan
- Zain Iqbal Allarakhia

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
