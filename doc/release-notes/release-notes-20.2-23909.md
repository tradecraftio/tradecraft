v20.2-23909 Release Notes
=========================

Freicoin version v20.2-23909 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v20.2-23909

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

From Freicoin v20 onwards, macOS versions earlier than 10.12 are no longer supported. Additionally, Freicoin does not yet change appearance when macOS "dark mode" is activated.

Known Bugs
----------

The process for generating the source code release ("tarball") has changed in an effort to make it more complete, however, there are a few regressions in this release:

- The generated `configure` script is currently missing, and you will need to install autotools and run `./autogen.sh` before you can run `./configure`.  This is the same as when checking out from git.

- Instead of running `make` simply, you should instead run `FREICOIN_GENBUILD_NO_GIT=1 make`.

Notable changes
---------------

### P2P and network changes

##### Removal of BIP61 reject network messages from Freicoin

The `-enablebip61` command line option to enable BIP61 has been removed.  (bitcoin/bitcoin#17004)

This feature has been disabled by default since Freicoin version v18.  Nodes on the network can not generally be trusted to send valid messages (including reject messages), so this should only ever be used when connected to a trusted node.  Please use the alternatives recommended below if you rely on this removed feature:

- Testing or debugging of implementations of the Freicoin P2P network protocol should be done by inspecting the log messages that are produced by a recent version of Freicoin.  Freicoin logs debug messages (`-debug=<category>`) to a stream (`-printtoconsole`) or to a file (`-debuglogfile=<debug.log>`).

- Testing the validity of a block can be achieved by specific RPCs:

  - `submitblock`

  - `getblocktemplate` with `'mode'` set to `'proposal'` for blocks with potentially invalid POW

- Testing the validity of a transaction can be achieved by specific RPCs:

  - `sendrawtransaction`

  - `testmempoolaccept`

- Wallets should not assume a transaction has propagated to the network just because there are no reject messages.  Instead, listen for the transaction to be announced by other peers on the network.  Wallets should not assume a lack of reject messages means a transaction pays an appropriate fee.  Instead, set fees using fee estimation and use replace-by-fee to increase a transaction's fee if it hasn't confirmed within the desired amount of time.

The removal of BIP61 reject message support also has the following minor RPC and logging implications:

- `testmempoolaccept` and `sendrawtransaction` no longer return the P2P reject code when a transaction is not accepted to the mempool.  They still return the verbal reject reason.

- Log messages that previously reported the reject code when a transaction was not accepted to the mempool now no longer report the reject code.  The reason for rejection is still reported.

### Changes regarding misbehaving peers

Peers that misbehave (e.g. send us invalid blocks) are now referred to as discouraged nodes in log output, as they're not (and weren't) strictly banned: incoming connections are still allowed from them, but they're preferred for eviction.

Furthermore, a few additional changes are introduced to how discouraged addresses are treated:

- Discouraging an address does not time out automatically after 24 hours (or the `-bantime` setting).  Depending on traffic from other peers, discouragement may time out at an indeterminate time.

- Discouragement is not persisted over restarts.

- There is no method to list discouraged addresses. They are not returned by the `listbanned` RPC.  That RPC also no longer reports the `ban_reason` field, as `"manually added"` is the only remaining option.

- Discouragement cannot be removed with the `setban remove` RPC command.  If you need to remove a discouragement, you can remove all discouragements by stop-starting your node.

### Updated RPCs

- The RPCs which accept descriptors now accept the new `sortedmulti(...)` descriptor type which supports multisig scripts where the public keys are sorted lexicographically in the resulting script.  (bitcoin/bitcoin#17056)

- The `walletprocesspsbt` and `walletcreatefundedpsbt` RPCs now include BIP32 derivation paths by default for public keys if we know them.  This can be disabled by setting the `bip32derivs` parameter to `false`.  (bitcoin/bitcoin#17264)

- The `bumpfee` RPC's parameter `totalFee`, which was deprecated in v19, has been removed.  (bitcoin/bitcoin#18312)

- The `bumpfee` RPC will return a PST when used with wallets that have private keys disabled.  (bitcoin/bitcoin#16373)

- The `getpeerinfo` RPC now includes a `mapped_as` field to indicate the mapped Autonomous System used for diversifying peer selection.  See the `-asmap` configuration option described below in _New Settings_.  (bitcoin/bitcoin#16702)

- The `createmultisig` and `addmultisigaddress` RPCs now return an output script descriptor for the newly created address.  (bitcoin/bitcoin#18032)

### Build System

- OpenSSL is no longer used by Freicoin.  (bitcoin/bitcoin#17265)

- BIP70 support has been fully removed from Freicoin.  The `--enable-bip70` option remains, but it will throw an error during configure.  (bitcoin/bitcoin#17165)

- glibc 2.17 or greater is now required to run the release binaries.  This retains compatibility with RHEL 7, CentOS 7, Debian 8 and Ubuntu 14.04 LTS.  (bitcoin/bitcoin#17538)

- The source code archives that are provided with gitian builds no longer contain any autotools artifacts.  Therefore, to build from such source, a user should run the `./autogen.sh` script from the root of the unpacked archive.  This implies that `autotools` and other required packages are installed on the user's system.  (bitcoin/bitcoin#18331)

### New settings

- New `rpcwhitelist` and `rpcwhitelistdefault` configuration parameters allow giving certain RPC users permissions to only some RPC calls.  (bitcoin/bitcoin#12763)

- A new `-asmap` configuration option has been added to diversify the node's network connections by mapping IP addresses Autonomous System Numbers (ASNs) and then limiting the number of connections made to any single ASN.  See [issue #16599](https://github.com/bitcoin/bitcoin/issues/16599), [PR #16702](https://github.com/bitcoin/bitcoin/pull/16702), and the `freicoind help` for more information.  This option is experimental and subject to removal or breaking changes in future releases, so the legacy /16 prefix mapping of IP addresses remains the default.  (bitcoin/bitcoin#16702)

### Updated settings

- All custom settings configured when Freicoin starts are now written to the `debug.log` file to assist troubleshooting.  (bitcoin/bitcoin#16115)

- Importing blocks upon startup via the `bootstrap.dat` file no longer occurs by default. The file must now be specified with `-loadblock=<file>`.  (bitcoin/bitcoin#17044)

- The `-debug=db` logging category has been renamed to `-debug=walletdb` to distinguish it from `coindb`.  The `-debug=db` option has been deprecated and will be removed in the next major release.  (bitcoin/bitcoin#17410)

- The `-walletnotify` configuration parameter will now replace any `%w` in its argument with the name of the wallet generating the notification.  This is not supported on Windows. (bitcoin/bitcoin#13339)

### Removed settings

- The `-whitelistforcerelay` configuration parameter has been removed after it was discovered that it was rendered ineffective in version 0.13 and hasn't actually been supported for almost four years.  (bitcoin/bitcoin#17985)

### GUI changes

- The "Start Freicoin on system login" option has been removed on macOS.  (bitcoin/bitcoin#17567)

- In the Peers window, the details for a peer now displays a `Mapped AS` field to indicate the mapped Autonomous System used for diversifying peer selection.  See the `-asmap` configuration option in _New Settings_, above.  (bitcoin/bitcoin#18402)

- A "known bug" [announced](https://bitcoincore.org/en/releases/0.18.0/#wallet-gui) in the release notes of version v18 has been fixed.  The issue affected anyone who simultaneously used multiple Freicoin wallets and the GUI coin control feature.  (bitcoin/bitcoin#18894)

- For watch-only wallets, creating a new transaction in the Send screen or fee bumping an existing transaction in the Transactions screen will automatically copy a Partially-Signed Transaction (PST) to the system clipboard.  This can then be pasted into an external program such as [HWI](https://github.com/bitcoin-core/HWI) for signing.  Future versions of Freicoin should support a GUI option for finalizing and broadcasting PSTs, but for now the debug console may be used with the `finalizepsbt` and `sendrawtransaction` RPCs.  (bitcoin/bitcoin#16944, bitcoin/bitcoin#17492)

### Wallet

- The wallet now by default uses bech32 addresses when using RPC, and creates native segwit change outputs.  (bitcoin/bitcoin#16884)

- The way that output trust was computed has been fixed, which affects confirmed/unconfirmed balance status and coin selection.  (bitcoin/bitcoin#16766)

- The `gettransaction`, `listtransactions` and `listsinceblock` RPC responses now also include the height of the block that contains the wallet transaction, if any.  (bitcoin/bitcoin#17437)

- The `getaddressinfo` RPC has had its `label` field deprecated (re-enable for this release using the configuration parameter `-deprecatedrpc=label`).  The `labels` field is altered from returning JSON objects to returning a JSON array of label names (re-enable previous behavior for this release using the configuration parameter `-deprecatedrpc=labelspurpose`).  Backwards compatibility using the deprecated configuration parameters is expected to be dropped in the v21 release.  (bitcoin/bitcoin#17585, bitcoin/bitcoin#17578)

### Notification changes

`-walletnotify` notifications are now sent for wallet transactions that are removed from the mempool because they conflict with a new block.  These notifications were sent previously before the v19 release, but had been broken since that release (bug [#18325](https://github.com/bitcoin/bitcoin/issues/18325)).

### PST changes

PSTs will contain both the non-witness utxo and the witness utxo for segwit inputs in order to restore compatibility with wallet software that are now requiring the full previous transaction for segwit inputs. The witness utxo is still provided to maintain compatibility with software which relied on its existence to determine whether an input was segwit.

Low-level changes
-----------------

### Utilities

- The `freicoin-cli` utility used with the `-getinfo` parameter now returns a `headers` field with the number of downloaded block headers on the best headers chain (similar to the `blocks` field that is also returned) and a `verificationprogress` field that estimates how much of the best block chain has been synced by the local node.  The information returned no longer includes the `protocolversion`, `walletversion`, and `keypoololdest` fields.  (bitcoin/bitcoin#17302, bitcoin/bitcoin#17650)

- The `freicoin-cli` utility now accepts a `-stdinwalletpassphrase` parameter that can be used when calling the `walletpassphrase` and `walletpassphrasechange` RPCs to read the passphrase from standard input without echoing it to the terminal, improving security against anyone who can look at your screen.  The existing `-stdinrpcpass` parameter is also updated to not echo the passphrase.  (bitcoin/bitcoin#13716)

### Command line

- Command line options prefixed with main/test/regtest network names like `-main.port=8333` `-test.server=1` previously were allowed but ignored.  Now they trigger "Invalid parameter" errors on startup.  (bitcoin/bitcoin#17482)

### New RPCs

- The `dumptxoutset` RPC outputs a serialized snapshot of the current UTXO set. A script is provided in the `contrib/devtools` directory for generating a snapshot of the UTXO set at a particular block height.  (bitcoin/bitcoin#16899)

- The `generatetodescriptor` RPC allows testers using regtest mode to generate blocks that pay an arbitrary output script descriptor.  (bitcoin/bitcoin#16943)

### Updated RPCs

- The `verifychain` RPC default values are now static instead of depending on the command line options or configuration file (`-checklevel`, and `-checkblocks`). Users can pass in the RPC arguments explicitly when they don't want to rely on the default values.  (bitcoin/bitcoin#18541)

- The `getblockchaininfo` RPC's `verificationprogress` field will no longer report values higher than 1.  Previously it would occasionally report the chain was more than 100% verified.  (bitcoin/bitcoin#17328)

### Tests

- It is now an error to use an unqualified `walletdir=path` setting in the config file if running on testnet or regtest networks.  The setting now needs to be qualified as `chain.walletdir=path` or placed in the appropriate `[chain]` section.  (bitcoin/bitcoin#17447)

- `-fallbackfee` was 0 (disabled) by default for the main chain, but 0.0002 by default for the test chains. Now it is 0 by default for all chains.  Testnet and regtest users will have to add `fallbackfee=0.0002` to their configuration if they weren't setting it and they want it to keep working like before.  (bitcoin/bitcoin#16524)

### Build system

- Support is provided for building with the Android Native Development Kit (NDK).  (bitcoin/bitcoin#16110)

v20.2-23909 Change log
----------------------

### Mining
- bitcoin/bitcoin#18742 miner: Avoid stack-use-after-return in validationinterface (MarcoFalke)
- bitcoin/bitcoin#19019 Fix GBT: Restore "!segwit" and "csv" to "rules" key (luke-jr)

### Block and transaction handling
- bitcoin/bitcoin#15283 log: Fix UB with bench on genesis block (instagibbs)
- bitcoin/bitcoin#16507 feefilter: Compute the absolute fee rather than stored rate (instagibbs)
- bitcoin/bitcoin#16688 log: Add validation interface logging (jkczyz)
- bitcoin/bitcoin#16805 log: Add timing information to FlushStateToDisk() (jamesob)
- bitcoin/bitcoin#16902 O(1) `OP_IF/NOTIF/ELSE/ENDIF` script implementation (sipa)
- bitcoin/bitcoin#16945 introduce CChainState::GetCoinsCacheSizeState (jamesob)
- bitcoin/bitcoin#16974 Walk pindexBestHeader back to ChainActive().Tip() if it is invalid (TheBlueMatt)
- bitcoin/bitcoin#17004 Remove REJECT code from CValidationState (jnewbery)
- bitcoin/bitcoin#17080 Explain why `fCheckDuplicateInputs` can not be skipped and remove it (MarcoFalke)
- bitcoin/bitcoin#17328 GuessVerificationProgress: cap the ratio to 1 (darosior)
- bitcoin/bitcoin#17399 Templatize ValidationState instead of subclassing (jkczyz)
- bitcoin/bitcoin#17407 node: Add reference to mempool in NodeContext (MarcoFalke)
- bitcoin/bitcoin#17708 prevector: Avoid misaligned member accesses (ajtowns)
- bitcoin/bitcoin#17850,#17896,#17957,#18021,#18021,#18112 Serialization improvements (sipa)
- bitcoin/bitcoin#17925 Improve UpdateTransactionsFromBlock with Epochs (JeremyRubin)
- bitcoin/bitcoin#18002 Abstract out script execution out of `VerifyWitnessProgram()` (sipa)
- bitcoin/bitcoin#18388 Make VerifyWitnessProgram use a Span stack (sipa)
- bitcoin/bitcoin#18433 serialization: prevent int overflow for big Coin::nHeight (pierreN)
- bitcoin/bitcoin#18500 chainparams: Bump assumed valid hash (MarcoFalke)
- bitcoin/bitcoin#18551 Do not clear validationinterface entries being executed (sipa)

### P2P protocol and network code
- bitcoin/bitcoin#15437 Remove BIP61 reject messages (MarcoFalke)
- bitcoin/bitcoin#16702 Supply and use asmap to improve IP bucketing in addrman (naumenkogs)
- bitcoin/bitcoin#16851 Continue relaying transactions after they expire from mapRelay (ajtowns)
- bitcoin/bitcoin#17164 Avoid allocating memory for addrKnown where we don't need it (naumenkogs)
- bitcoin/bitcoin#17243 tools: add PoissonNextSend method that returns mockable time (amitiuttarwar)
- bitcoin/bitcoin#17251 SocketHandler logs peer id for close and disconnect (Sjors)
- bitcoin/bitcoin#17573 Seed RNG with precision timestamps on receipt of net messages (TheBlueMatt)
- bitcoin/bitcoin#17624 Fix an uninitialized read in ProcessMessage(…, "tx", …) when receiving a transaction we already have (practicalswift)
- bitcoin/bitcoin#17754 Don't allow resolving of std::string with embedded NUL characters. Add tests (practicalswift)
- bitcoin/bitcoin#17758 Fix CNetAddr::IsRFC2544 comment + tests (tynes)
- bitcoin/bitcoin#17812 config, net, test: Asmap feature refinements and functional tests (jonatack)
- bitcoin/bitcoin#17951 Use rolling bloom filter of recent block txs for AlreadyHave() check (sdaftuar)
- bitcoin/bitcoin#17985 Remove forcerelay of rejected txs (MarcoFalke)
- bitcoin/bitcoin#18023 Fix some asmap issues (sipa)
- bitcoin/bitcoin#18054 Reference instead of copy in BlockConnected range loop (jonatack)
- bitcoin/bitcoin#18376 Fix use-after-free in tests (vasild)
- bitcoin/bitcoin#18454 Make addr relay mockable, add test (MarcoFalke)
- bitcoin/bitcoin#18458 Add missing `cs_vNodes` lock (MarcoFalke)
- bitcoin/bitcoin#18506 Hardcoded seeds update for 0.20 (laanwj)
- bitcoin/bitcoin#18808 Drop unknown types in getdata (jnewbery)
- bitcoin/bitcoin#18962 Only send a getheaders for one block in an INV (jnewbery)
- bitcoin/bitcoin#19219 Replace automatic bans with discouragement filter (sipa)
- bitcoin/bitcoin#19620 Add txids with non-standard inputs to reject filter (sdaftuar)
- bitcoin/bitcoin#20146 Send post-verack handshake messages at most once (MarcoFalke)

### Wallet
- bitcoin/bitcoin#13339 Replace %w by wallet name in -walletnotify script (promag)
- bitcoin/bitcoin#15931 Remove GetDepthInMainChain dependency on locked chain interface (ariard)
- bitcoin/bitcoin#16373 bumpfee: Return PSBT when wallet has privkeys disabled (instagibbs)
- bitcoin/bitcoin#16524 Disable -fallbackfee by default (jtimon)
- bitcoin/bitcoin#16766 Make IsTrusted scan parents recursively (JeremyRubin)
- bitcoin/bitcoin#16884 Change default address type to bech32 (instagibbs)
- bitcoin/bitcoin#16911 Only check the hash of transactions loaded from disk (achow101)
- bitcoin/bitcoin#16923 Handle duplicate fileid exception (promag)
- bitcoin/bitcoin#17056 descriptors: Introduce sortedmulti descriptor (achow101)
- bitcoin/bitcoin#17070 Avoid showing GUI popups on RPC errors (MarcoFalke)
- bitcoin/bitcoin#17138 Remove wallet access to some node arguments (jnewbery)
- bitcoin/bitcoin#17237 LearnRelatedScripts only if KeepDestination (promag)
- bitcoin/bitcoin#17260 Split some CWallet functions into new LegacyScriptPubKeyMan (achow101)
- bitcoin/bitcoin#17261 Make ScriptPubKeyMan an actual interface and the wallet to have multiple (achow101)
- bitcoin/bitcoin#17290 Enable BnB coin selection for preset inputs and subtract fee from outputs (achow101)
- bitcoin/bitcoin#17373 Various fixes and cleanup to keypool handling in LegacyScriptPubKeyMan and CWallet (achow101)
- bitcoin/bitcoin#17410 Rename `db` log category to `walletdb` (like `coindb`) (laanwj)
- bitcoin/bitcoin#17444 Avoid showing GUI popups on RPC errors (take 2) (MarcoFalke)
- bitcoin/bitcoin#17447 Make -walletdir network only (promag)
- bitcoin/bitcoin#17537 Cleanup and move opportunistic and superfluous TopUp()s (achow101)
- bitcoin/bitcoin#17553 Remove out of date comments for CalculateMaximumSignedTxSize (instagibbs)
- bitcoin/bitcoin#17568 Fix when sufficient preset inputs and subtractFeeFromOutputs (achow101)
- bitcoin/bitcoin#17677 Activate watchonly wallet behavior for LegacySPKM only (instagibbs)
- bitcoin/bitcoin#17719 Document better -keypool as a look-ahead safety mechanism (ariard)
- bitcoin/bitcoin#17843 Reset reused transactions cache (fjahr)
- bitcoin/bitcoin#17889 Improve CWallet:MarkDestinationsDirty (promag)
- bitcoin/bitcoin#18034 Get the OutputType for a descriptor (achow101)
- bitcoin/bitcoin#18067 Improve LegacyScriptPubKeyMan::CanProvide script recognition (ryanofsky)
- bitcoin/bitcoin#18115 Pass in transactions and messages for signing instead of exporting the private keys (achow101)
- bitcoin/bitcoin#18192,#18546 Bugfix: Wallet: Safely deal with change in the address book (luke-jr)
- bitcoin/bitcoin#18204 descriptors: Improve descriptor cache and cache xpubs (achow101)
- bitcoin/bitcoin#18274 rpc/wallet: Initialize nFeeRequired to avoid using garbage value on failure (kallewoof)
- bitcoin/bitcoin#18312 Remove deprecated fee bumping by totalFee (jonatack)
- bitcoin/bitcoin#18338 Fix wallet unload race condition (promag)
- bitcoin/bitcoin#19300 Handle concurrent wallet loading (promag)
- bitcoin/bitcoin#18982 Minimal fix to restore conflicted transaction notifications (ryanofsky)
- bitcoin/bitcoin#19740 Simplify and fix CWallet::SignTransaction (achow101)

### RPC and other APIs
- bitcoin/bitcoin#12763 Add RPC Whitelist Feature from bitcoin/bitcoin#12248 (JeremyRubin)
- bitcoin/bitcoin#13716 cli: `-stdinwalletpassphrase` and non-echo stdin passwords (kallewoof)
- bitcoin/bitcoin#16689 Add missing fields to wallet rpc help output (ariard)
- bitcoin/bitcoin#16821 Fix bug where duplicate PSBT keys are accepted (erasmospunk)
- bitcoin/bitcoin#16899 UTXO snapshot creation (dumptxoutset)
- bitcoin/bitcoin#17156 psbt: Check that various indexes and amounts are within bounds (achow101)
- bitcoin/bitcoin#17264 Set default bip32derivs to true for psbt methods (Sjors)
- bitcoin/bitcoin#17283 improve getaddressinfo test coverage, help, code docs (jonatack)
- bitcoin/bitcoin#17302 cli: Add "headers" and "verificationprogress" to -getinfo (laanwj)
- bitcoin/bitcoin#17318 replace asserts in RPC code with `CHECK_NONFATAL` and add linter (adamjonas)
- bitcoin/bitcoin#17437 Expose block height of wallet transactions (promag)
- bitcoin/bitcoin#17519 Remove unused `COINBASE_FLAGS` (narula)
- bitcoin/bitcoin#17578 Simplify getaddressinfo labels, deprecate previous behavior (jonatack)
- bitcoin/bitcoin#17585 deprecate getaddressinfo label (jonatack)
- bitcoin/bitcoin#17746 Remove vector copy from listtransactions (promag)
- bitcoin/bitcoin#17809 Auto-format RPCResult (MarcoFalke)
- bitcoin/bitcoin#18032 Output a descriptor in createmultisig and addmultisigaddress (achow101)
- bitcoin/bitcoin#18122 Update validateaddress RPCExamples to bech32 (theStack)
- bitcoin/bitcoin#18208 Change RPCExamples to bech32 (yusufsahinhamza)
- bitcoin/bitcoin#18268 Remove redundant types from descriptions (docallag)
- bitcoin/bitcoin#18346 Document an RPCResult for all calls; Enforce at compile time (MarcoFalke)
- bitcoin/bitcoin#18396 Add missing HelpExampleRpc for getblockfilter (theStack)
- bitcoin/bitcoin#18398 Fix broken RPCExamples for waitforblock(height) (theStack)
- bitcoin/bitcoin#18444 Remove final comma for last entry of fixed-size arrays/objects in RPCResult (luke-jr)
- bitcoin/bitcoin#18459 Remove unused getbalances() code (jonatack)
- bitcoin/bitcoin#18484 Correctly compute redeemScript from witnessScript for signrawtransaction (achow101)
- bitcoin/bitcoin#18487 Fix rpcRunLater race in walletpassphrase (promag)
- bitcoin/bitcoin#18499 Make rpc documentation not depend on call-time rpc args (MarcoFalke)
- bitcoin/bitcoin#18532 Avoid initialization-order-fiasco on static CRPCCommand tables (MarcoFalke)
- bitcoin/bitcoin#18541 Make verifychain default values static, not depend on global args (MarcoFalke)
- bitcoin/bitcoin#18809 Do not advertise dumptxoutset as a way to flush the chainstate (MarcoFalke)
- bitcoin/bitcoin#18814 Relock wallet only if most recent callback (promag)
- bitcoin/bitcoin#19524 Increment input value sum only once per UTXO in decodepsbt (fanquake)
- bitcoin/bitcoin#19517 psbt: Increment input value sum only once per UTXO in decodepsbt (achow101)
- bitcoin/bitcoin#19215 psbt: Include and allow both non_witness_utxo and witness_utxo for segwit inputs (achow101)
- bitcoin/bitcoin#19836 Properly deserialize txs with witness before signing (MarcoFalke)
- bitcoin/bitcoin#20731 Add missing description of vout in getrawtransaction help text (benthecarman)

### GUI
- bitcoin/bitcoin#15023 Restore RPC Console to non-wallet tray icon menu (luke-jr)
- bitcoin/bitcoin#15084 Don't disable the sync overlay when wallet is disabled (benthecarman)
- bitcoin/bitcoin#15098 Show addresses for "SendToSelf" transactions (hebasto)
- bitcoin/bitcoin#15756 Add shortcuts for tab tools (promag)
- bitcoin/bitcoin#16944 create PSBT with watch-only wallet (Sjors)
- bitcoin/bitcoin#16964 Change sendcoins dialogue Yes to Send (instagibbs)
- bitcoin/bitcoin#17068 Always generate `bitcoinstrings.cpp` on `make translate` (D4nte)
- bitcoin/bitcoin#17096 Rename debug window (Zero-1729)
- bitcoin/bitcoin#17105 Make RPCConsole::TabTypes an enum class (promag)
- bitcoin/bitcoin#17125 Add toolTip and placeholderText to sign message fields (dannmat)
- bitcoin/bitcoin#17165 Remove BIP70 support (fanquake)
- bitcoin/bitcoin#17180 Improved tooltip for send amount field (JeremyCrookshank)
- bitcoin/bitcoin#17186 Add placeholder text to the sign message field (Danny-Scott)
- bitcoin/bitcoin#17195 Send amount placeholder value (JeremyCrookshank)
- bitcoin/bitcoin#17226 Fix payAmount tooltip in SendCoinsEntry (promag)
- bitcoin/bitcoin#17360 Cleaning up hide button tool tip (Danny-Scott)
- bitcoin/bitcoin#17446 Changed tooltip for 'Label' & 'Message' text fields to be more clear (dannmat)
- bitcoin/bitcoin#17453 Fix intro dialog labels when the prune button is toggled (hebasto)
- bitcoin/bitcoin#17474 Bugfix: GUI: Recognise `NETWORK_LIMITED` in formatServicesStr (luke-jr)
- bitcoin/bitcoin#17492 Bump fee returns PSBT on clipboard for watchonly-only wallets (instagibbs)
- bitcoin/bitcoin#17567 Remove macOS start on login code (fanquake)
- bitcoin/bitcoin#17587 Show watch-only balance in send screen (Sjors)
- bitcoin/bitcoin#17694 Disable 3rd-party tx-urls when wallet disabled (brakmic)
- bitcoin/bitcoin#17696 Force set nPruneSize in QSettings after the intro dialog (hebasto)
- bitcoin/bitcoin#17702 Move static placeholder texts to forms (laanwj)
- bitcoin/bitcoin#17826 Log Qt related info (hebasto)
- bitcoin/bitcoin#17886 Restore English translation option (achow101)
- bitcoin/bitcoin#17906 Set CConnman byte counters earlier to avoid uninitialized reads (ryanofsky)
- bitcoin/bitcoin#17935 Hide HD & encryption icons when no wallet loaded (brakmic)
- bitcoin/bitcoin#17998 Shortcut to close ModalOverlay (emilengler)
- bitcoin/bitcoin#18007 Bugfix: GUI: Hide the HD/encrypt icons earlier so they get re-shown if another wallet is open (luke-jr)
- bitcoin/bitcoin#18060 Drop PeerTableModel dependency to ClientModel (promag)
- bitcoin/bitcoin#18062 Fix unintialized WalletView::progressDialog (promag)
- bitcoin/bitcoin#18091 Pass clientmodel changes from walletframe to walletviews (jonasschnelli)
- bitcoin/bitcoin#18101 Fix deprecated QCharRef usage (hebasto)
- bitcoin/bitcoin#18121 Throttle GUI update pace when -reindex (hebasto)
- bitcoin/bitcoin#18123 Fix race in WalletModel::pollBalanceChanged (ryanofsky)
- bitcoin/bitcoin#18160 Avoid Wallet::GetBalance in WalletModel::pollBalanceChanged (promag)
- bitcoin/bitcoin#18360 Bump transifex slug and update English translations for 0.20 (laanwj)
- bitcoin/bitcoin#18402 Display mapped AS in peers info window (jonatack)
- bitcoin/bitcoin#18492 Translations update pre-branch (laanwj)
- bitcoin/bitcoin#18549 Fix Window -> Minimize menu item (hebasto)
- bitcoin/bitcoin#18578 Fix leak in CoinControlDialog::updateView (promag)
- bitcoin/bitcoin#18894 Fix manual coin control with multiple wallets loaded (promag)
- bitcoin/bitcoin#19097 Add missing QPainterPath include (achow101)
- bitcoin/bitcoin#19059 update Qt base translations for macOS release (fanquake)

### Build system
- bitcoin/bitcoin#16667 Remove mingw linker workaround from win gitian descriptor (fanquake)
- bitcoin/bitcoin#16669 Use new fork of osslsigncode for windows gitian signing (fanquake)
- bitcoin/bitcoin#16949 Only pass --disable-dependency-tracking to packages that understand it (fanquake)
- bitcoin/bitcoin#17008 Bump libevent to 2.1.11 in depends (stefanwouldgo)
- bitcoin/bitcoin#17029 gitian: Various improvements for windows descriptor (dongcarl)
- bitcoin/bitcoin#17033 Disable _FORTIFY_SOURCE when enable-debug (achow101)
- bitcoin/bitcoin#17057 Switch to upstream libdmg-hfsplus (fanquake)
- bitcoin/bitcoin#17066 Remove workaround for ancient libtool (hebasto)
- bitcoin/bitcoin#17074 Added double quotes (mztriz)
- bitcoin/bitcoin#17087 Add variable printing target to Makefiles (dongcarl)
- bitcoin/bitcoin#17118 depends macOS: point --sysroot to SDK (Sjors)
- bitcoin/bitcoin#17231 Fix boost mac cross build with clang 9+ (theuni)
- bitcoin/bitcoin#17265 Remove OpenSSL (fanquake)
- bitcoin/bitcoin#17284 Update retry to current version (RandyMcMillan)
- bitcoin/bitcoin#17308 nsis: Write to correct filename in first place (dongcarl)
- bitcoin/bitcoin#17324,#18099 Update univalue subtree (MarcoFalke)
- bitcoin/bitcoin#17398 Update leveldb to 1.22+ (laanwj)
- bitcoin/bitcoin#17409 Avoid hardcoded libfaketime dir in gitian (MarcoFalke)
- bitcoin/bitcoin#17466 Fix C{,XX} pickup (dongcarl)
- bitcoin/bitcoin#17483 Set gitian arch back to amd64 (MarcoFalke)
- bitcoin/bitcoin#17486 Make Travis catch unused variables (Sjors)
- bitcoin/bitcoin#17538 Bump minimum libc to 2.17 for release binaries (fanquake)
- bitcoin/bitcoin#17542 Create test utility library from src/test/util/ (brakmic)
- bitcoin/bitcoin#17545 Remove libanl.so.1 from ALLOWED_LIBRARIES (fanquake)
- bitcoin/bitcoin#17547 Fix configure report about qr (hebasto)
- bitcoin/bitcoin#17569 Allow export of environ symbols and work around rv64 toolchain issue (laanwj)
- bitcoin/bitcoin#17647 lcov: filter depends from coverage reports (nijynot)
- bitcoin/bitcoin#17658 Add ability to skip building qrencode (fanquake)
- bitcoin/bitcoin#17678 Support for S390X and POWER targets (MarcoFalke)
- bitcoin/bitcoin#17682 util: Update tinyformat to upstream (laanwj)
- bitcoin/bitcoin#17698 Don't configure `xcb_proto` (fanquake)
- bitcoin/bitcoin#17730 Remove Qt networking features (fanquake)
- bitcoin/bitcoin#17738 Remove linking librt for backwards compatibility (fanquake)
- bitcoin/bitcoin#17740 Remove configure checks for win libraries we don't link against (fanquake)
- bitcoin/bitcoin#17741 Included `test_bitcoin-qt` in msvc build (sipsorcery)
- bitcoin/bitcoin#17756 Remove `WINDOWS_BITS` from build system (fanquake)
- bitcoin/bitcoin#17769 Set `AC_PREREQ` to 2.69 (fanquake)
- bitcoin/bitcoin#17880 Add -Wdate-time to Werror flags (fanquake)
- bitcoin/bitcoin#17910 Remove double `LIBBITCOIN_SERVER` linking (fanquake)
- bitcoin/bitcoin#17928 Consistent use of package variable (Bushstar)
- bitcoin/bitcoin#17933 guix: Pin Guix using `guix time-machine` (dongcarl)
- bitcoin/bitcoin#17948 pass -fno-ident in Windows gitian descriptor (fanquake)
- bitcoin/bitcoin#18003 Remove --large-address-aware linker flag (fanquake)
- bitcoin/bitcoin#18004 Don't embed a build-id when building libdmg-hfsplus (fanquake)
- bitcoin/bitcoin#18051 Fix behavior when `ALLOW_HOST_PACKAGES` unset (hebasto)
- bitcoin/bitcoin#18059 Add missing attributes to Win installer (fanquake)
- bitcoin/bitcoin#18104 Skip i686 build by default in guix and gitian (MarcoFalke)
- bitcoin/bitcoin#18107 Add `cov_fuzz` target (MarcoFalke)
- bitcoin/bitcoin#18135 Add --enable-determinism configure flag (fanquake)
- bitcoin/bitcoin#18145 Add Wreturn-type to Werror flags, check on more Travis machines (Sjors)
- bitcoin/bitcoin#18264 Remove Boost Chrono (fanquake)
- bitcoin/bitcoin#18290 Set minimum Automake version to 1.13 (hebasto)
- bitcoin/bitcoin#18320 guix: Remove now-unnecessary gcc make flag (dongcarl)
- bitcoin/bitcoin#18331 Use git archive as source tarball (hebasto)
- bitcoin/bitcoin#18397 Fix libevent linking for `bench_bitcoin` binary (hebasto)
- bitcoin/bitcoin#18426 scripts: `Previous_release`: improve behaviour on failed download (theStack)
- bitcoin/bitcoin#18429 Remove double `LIBBITCOIN_SERVER` from bench-Makefile (brakmic)
- bitcoin/bitcoin#18528 Create `test_fuzz` library from src/test/fuzz/fuzz.cpp (brakmic)
- bitcoin/bitcoin#18558 Fix boost detection for arch armv7l (hebasto)
- bitcoin/bitcoin#18598 gitian: Add missing automake package to gitian-win-signer.yml (achow101)
- bitcoin/bitcoin#18676 Check libevent minimum version in configure script (hebasto)
- bitcoin/bitcoin#18945 Ensure source tarball has leading directory name (laanwj)
- bitcoin/bitcoin#19152 improve build OS configure output (skmcontrib)
- bitcoin/bitcoin#19536 qt, build: Fix QFileDialog for static builds (hebasto)
- bitcoin/bitcoin#20142 build: set minimum required Boost to 1.48.0 (fanquake)
- bitcoin/bitcoin#20298 use the new plistlib API (jonasschnelli)
- bitcoin/bitcoin#20880 gitian: Use custom MacOS code signing tool (achow101)
- bitcoin/bitcoin#22190 Use latest signapple commit (achow101)

### Platform support
- bitcoin/bitcoin#16110 Add Android NDK support (icota)
- bitcoin/bitcoin#16392 macOS toolchain update (fanquake)
- bitcoin/bitcoin#16569 Increase init file stop timeout (setpill)
- bitcoin/bitcoin#17151 Remove OpenSSL PRNG seeding (Windows, Qt only) (fanquake)
- bitcoin/bitcoin#17365 Update README.md with working Android targets and API levels (icota)
- bitcoin/bitcoin#17521 Only use D-Bus with Qt on linux (fanquake)
- bitcoin/bitcoin#17550 Set minimum supported macOS to 10.12 (fanquake)
- bitcoin/bitcoin#17592 Appveyor install libevent[thread] vcpkg (sipsorcery)
- bitcoin/bitcoin#17660 Remove deprecated key from macOS Info.plist (fanquake)
- bitcoin/bitcoin#17663 Pass `-dead_strip_dylibs` to ld on macOS (fanquake)
- bitcoin/bitcoin#17676 Don't use OpenGL in Qt on macOS (fanquake)
- bitcoin/bitcoin#17686 Add `-bind_at_load` to macOS hardened LDFLAGS (fanquake)
- bitcoin/bitcoin#17787 scripts: Add macho pie check to security-check.py (fanquake)
- bitcoin/bitcoin#17800 random: don't special case clock usage on macOS (fanquake)
- bitcoin/bitcoin#17863 scripts: Add macho dylib checks to symbol-check.py (fanquake)
- bitcoin/bitcoin#17899 msvc: Ignore msvc linker warning and update to msvc build instructions (sipsorcery)
- bitcoin/bitcoin#17916 windows: Enable heap terminate-on-corruption (fanquake)
- bitcoin/bitcoin#18082 logging: Enable `thread_local` usage on macos (fanquake)
- bitcoin/bitcoin#18108 Fix `.gitignore` policy in `build_msvc` directory (hebasto)
- bitcoin/bitcoin#18295 scripts: Add macho lazy bindings check to security-check.py (fanquake)
- bitcoin/bitcoin#18358 util: Fix compilation with mingw-w64 7.0.0 (fanquake)
- bitcoin/bitcoin#18359 Fix sysctl() detection on macOS (fanquake)
- bitcoin/bitcoin#18364 random: remove getentropy() fallback for macOS < 10.12 (fanquake)
- bitcoin/bitcoin#18395 scripts: Add pe dylib checking to symbol-check.py (fanquake)
- bitcoin/bitcoin#18415 scripts: Add macho tests to test-security-check.py (fanquake)
- bitcoin/bitcoin#18425 releases: Update with new Windows code signing certificate (achow101)
- bitcoin/bitcoin#18702 Fix ASLR for bitcoin-cli on Windows (fanquake)

### Tests and QA
- bitcoin/bitcoin#12134 Build previous releases and run functional tests (Sjors)
- bitcoin/bitcoin#13693 Add coverage to estimaterawfee and estimatesmartfee (Empact)
- bitcoin/bitcoin#13728 lint: Run the ci lint stage on mac (Empact)
- bitcoin/bitcoin#15443 Add getdescriptorinfo functional test (promag)
- bitcoin/bitcoin#15888 Add `wallet_implicitsegwit` to test the ability to transform keys between address types (luke-jr)
- bitcoin/bitcoin#16540 Add `ASSERT_DEBUG_LOG` to unit test framework (MarcoFalke)
- bitcoin/bitcoin#16597 travis: Run full test suite on native macos (Sjors)
- bitcoin/bitcoin#16681 Use self.chain instead of 'regtest' in all current tests (jtimon)
- bitcoin/bitcoin#16786 add unit test for wallet watch-only methods involving PubKeys (theStack)
- bitcoin/bitcoin#16943 Add generatetodescriptor RPC (MarcoFalke)
- bitcoin/bitcoin#16973 Fix `combine_logs.py` for AppVeyor build (mzumsande)
- bitcoin/bitcoin#16975 Show debug log on unit test failure (MarcoFalke)
- bitcoin/bitcoin#16978 Seed test RNG context for each test case, print seed (MarcoFalke)
- bitcoin/bitcoin#17009, bitcoin/bitcoin#17018, bitcoin/bitcoin#17050, bitcoin/bitcoin#17051, bitcoin/bitcoin#17071, bitcoin/bitcoin#17076, bitcoin/bitcoin#17083, bitcoin/bitcoin#17093, bitcoin/bitcoin#17109, bitcoin/bitcoin#17113, bitcoin/bitcoin#17136, bitcoin/bitcoin#17229, bitcoin/bitcoin#17291, bitcoin/bitcoin#17357, bitcoin/bitcoin#17771, bitcoin/bitcoin#17777, bitcoin/bitcoin#17917, bitcoin/bitcoin#17926, bitcoin/bitcoin#17972, bitcoin/bitcoin#17989, bitcoin/bitcoin#17996, bitcoin/bitcoin#18009, bitcoin/bitcoin#18029, bitcoin/bitcoin#18047, bitcoin/bitcoin#18126, bitcoin/bitcoin#18176, bitcoin/bitcoin#18206, bitcoin/bitcoin#18353, bitcoin/bitcoin#18363, bitcoin/bitcoin#18407, bitcoin/bitcoin#18417, bitcoin/bitcoin#18423, bitcoin/bitcoin#18445, bitcoin/bitcoin#18455, bitcoin/bitcoin#18565 Add fuzzing harnesses (practicalswift)
- bitcoin/bitcoin#17011 ci: Use busybox utils for one build (MarcoFalke)
- bitcoin/bitcoin#17030 Fix Python Docstring to include all Args (jbampton)
- bitcoin/bitcoin#17041 ci: Run tests on arm (MarcoFalke)
- bitcoin/bitcoin#17069 Pass fuzzing inputs as constant references (practicalswift)
- bitcoin/bitcoin#17091 Add test for loadblock option and linearize scripts (fjahr)
- bitcoin/bitcoin#17108 fix "tx-size-small" errors after default address change (theStack)
- bitcoin/bitcoin#17121 Speed up `wallet_backup` by whitelisting peers (immediate tx relay) (theStack)
- bitcoin/bitcoin#17124 Speed up `wallet_address_types` by whitelisting peers (immediate tx relay) (theStack)
- bitcoin/bitcoin#17140 Fix bug in `blockfilter_index_tests` (jimpo)
- bitcoin/bitcoin#17199 use default address type (bech32) for `wallet_bumpfee` tests (theStack)
- bitcoin/bitcoin#17205 ci: Enable address sanitizer (asan) stack-use-after-return checking (practicalswift)
- bitcoin/bitcoin#17206 Add testcase to simulate bitcoin schema in leveldb (adamjonas)
- bitcoin/bitcoin#17209 Remove no longer needed UBSan suppressions (issues fixed). Add documentation (practicalswift)
- bitcoin/bitcoin#17220 Add unit testing for the CompressScript function (adamjonas)
- bitcoin/bitcoin#17225 Test serialisation as part of deserialisation fuzzing. Test round-trip equality where possible (practicalswift)
- bitcoin/bitcoin#17228 Add RegTestingSetup to `setup_common` (MarcoFalke)
- bitcoin/bitcoin#17233 travis: Run unit and functional tests on native arm (MarcoFalke)
- bitcoin/bitcoin#17235 Skip unnecessary fuzzer initialisation. Hold ECCVerifyHandle only when needed (practicalswift)
- bitcoin/bitcoin#17240 ci: Disable functional tests on mac host (MarcoFalke)
- bitcoin/bitcoin#17254 Fix `script_p2sh_tests` `OP_PUSHBACK2/4` missing (adamjonas)
- bitcoin/bitcoin#17267 bench: Fix negative values and zero for -evals flag (nijynot)
- bitcoin/bitcoin#17275 pubkey: Assert CPubKey's ECCVerifyHandle precondition (practicalswift)
- bitcoin/bitcoin#17288 Added TestWrapper class for interactive Python environments (jachiang)
- bitcoin/bitcoin#17292 Add new mempool benchmarks for a complex pool (JeremyRubin)
- bitcoin/bitcoin#17299 add reason checks for non-standard txs in `test_IsStandard` (theStack)
- bitcoin/bitcoin#17322 Fix input size assertion in `wallet_bumpfee.py` (instagibbs)
- bitcoin/bitcoin#17327 Add `rpc_fundrawtransaction` logging (jonatack)
- bitcoin/bitcoin#17330 Add `shrinkdebugfile=0` to regtest bitcoin.conf (sdaftuar)
- bitcoin/bitcoin#17340 Speed up fundrawtransaction test (jnewbery)
- bitcoin/bitcoin#17345 Do not instantiate CAddrDB for static call CAddrDB::Read() (hebasto)
- bitcoin/bitcoin#17362 Speed up `wallet_avoidreuse`, add logging (jonatack)
- bitcoin/bitcoin#17363 add "diamond" unit test to MempoolAncestryTests (theStack)
- bitcoin/bitcoin#17366 Reset global args between test suites (MarcoFalke)
- bitcoin/bitcoin#17367 ci: Run non-cross-compile builds natively (MarcoFalke)
- bitcoin/bitcoin#17378 TestShell: Fix typos & implement cleanups (jachiang)
- bitcoin/bitcoin#17384 Create new test library (MarcoFalke)
- bitcoin/bitcoin#17387 `wallet_importmulti`: use addresses of the same type as being imported (achow101)
- bitcoin/bitcoin#17388 Add missing newline in `util_ChainMerge` test (ryanofsky)
- bitcoin/bitcoin#17390 Add `util_ArgParsing` test (ryanofsky)
- bitcoin/bitcoin#17420 travis: Rework `cache_err_msg` (MarcoFalke)
- bitcoin/bitcoin#17423 ci: Make ci system read-only on the git work tree (MarcoFalke)
- bitcoin/bitcoin#17435 check custom ancestor limit in `mempool_packages.py` (theStack)
- bitcoin/bitcoin#17455 Update valgrind suppressions (practicalswift)
- bitcoin/bitcoin#17461 Check custom descendant limit in `mempool_packages.py` (theStack)
- bitcoin/bitcoin#17469 Remove fragile `assert_memory_usage_stable` (MarcoFalke)
- bitcoin/bitcoin#17470 ci: Use clang-8 for fuzzing to run on aarch64 ci systems (MarcoFalke)
- bitcoin/bitcoin#17480 Add unit test for non-standard txs with too large scriptSig (theStack)
- bitcoin/bitcoin#17497 Skip tests when utils haven't been compiled (fanquake)
- bitcoin/bitcoin#17502 Add unit test for non-standard bare multisig txs (theStack)
- bitcoin/bitcoin#17511 Add bounds checks before base58 decoding (sipa)
- bitcoin/bitcoin#17517 ci: Bump to clang-8 for asan build to avoid segfaults on ppc64le (MarcoFalke)
- bitcoin/bitcoin#17522 Wait until mempool is loaded in `wallet_abandonconflict` (MarcoFalke)
- bitcoin/bitcoin#17532 Add functional test for non-standard txs with too large scriptSig (theStack)
- bitcoin/bitcoin#17541 Add functional test for non-standard bare multisig txs (theStack)
- bitcoin/bitcoin#17555 Add unit test for non-standard txs with wrong nVersion (dspicher)
- bitcoin/bitcoin#17571 Add `libtest_util` library to msvc build configuration (sipsorcery)
- bitcoin/bitcoin#17591 ci: Add big endian platform - s390x (elichai)
- bitcoin/bitcoin#17593 Move more utility functions into test utility library (mzumsande)
- bitcoin/bitcoin#17633 Add option --valgrind to run the functional tests under Valgrind (practicalswift)
- bitcoin/bitcoin#17635 ci: Add centos 7 build (hebasto)
- bitcoin/bitcoin#17641 Add unit test for leveldb creation with unicode path (sipsorcery)
- bitcoin/bitcoin#17674 Add initialization order fiasco detection in Travis (practicalswift)
- bitcoin/bitcoin#17675 Enable tests which are incorrectly skipped when running `test_runner.py --usecli` (practicalswift)
- bitcoin/bitcoin#17685 Fix bug in the descriptor parsing fuzzing harness (`descriptor_parse`) (practicalswift)
- bitcoin/bitcoin#17705 re-enable CLI test support by using EncodeDecimal in json.dumps() (fanquake)
- bitcoin/bitcoin#17720 add unit test for non-standard "scriptsig-not-pushonly" txs (theStack)
- bitcoin/bitcoin#17767 ci: Fix qemu issues (MarcoFalke)
- bitcoin/bitcoin#17793 ci: Update github actions ci vcpkg cache on msbuild update (hebasto)
- bitcoin/bitcoin#17806 Change filemode of `rpc_whitelist.py` (emilengler)
- bitcoin/bitcoin#17849 ci: Fix brew python link (hebasto)
- bitcoin/bitcoin#17851 Add `std::to_string` to list of locale dependent functions (practicalswift)
- bitcoin/bitcoin#17893 Fix double-negative arg test (hebasto)
- bitcoin/bitcoin#17900 ci: Combine 32-bit build with centos 7 build (theStack)
- bitcoin/bitcoin#17921 Test `OP_CSV` empty stack fail in `feature_csv_activation.py` (theStack)
- bitcoin/bitcoin#17931 Fix `p2p_invalid_messages` failing in Python 3.8 because of warning (elichai)
- bitcoin/bitcoin#17947 add unit test for non-standard txs with too large tx size (theStack)
- bitcoin/bitcoin#17959 Check specific reject reasons in `feature_csv_activation.py` (theStack)
- bitcoin/bitcoin#17984 Add p2p test for forcerelay permission (MarcoFalke)
- bitcoin/bitcoin#18001 Updated appveyor job to checkout a specific vcpkg commit ID (sipsorcery)
- bitcoin/bitcoin#18008 fix fuzzing using libFuzzer on macOS (fanquake)
- bitcoin/bitcoin#18013 bench: Fix benchmarks filters (elichai)
- bitcoin/bitcoin#18018 reset fIsBareMultisigStd after bare-multisig tests (fanquake)
- bitcoin/bitcoin#18022 Fix appveyor `test_bitcoin` build of `*.raw` (MarcoFalke)
- bitcoin/bitcoin#18037 util: Allow scheduler to be mocked (amitiuttarwar)
- bitcoin/bitcoin#18056 ci: Check for submodules (emilengler)
- bitcoin/bitcoin#18069 Replace 'regtest' leftovers by self.chain (theStack)
- bitcoin/bitcoin#18081 Set a name for CI Docker containers (fanquake)
- bitcoin/bitcoin#18109 Avoid hitting some known minor tinyformat issues when fuzzing strprintf(…) (practicalswift)
- bitcoin/bitcoin#18155 Add harness which fuzzes EvalScript and VerifyScript using a fuzzed signature checker (practicalswift)
- bitcoin/bitcoin#18159 Add --valgrind option to `test/fuzz/test_runner.py` for running fuzzing test cases under valgrind (practicalswift)
- bitcoin/bitcoin#18166 ci: Run fuzz testing test cases (bitcoin-core/qa-assets) under valgrind to catch memory errors (practicalswift)
- bitcoin/bitcoin#18172 Transaction expiry from mempool (0xB10C)
- bitcoin/bitcoin#18181 Remove incorrect assumptions in `validation_flush_tests` (MarcoFalke)
- bitcoin/bitcoin#18183 Set `catch_system_errors=no` on boost unit tests (MarcoFalke)
- bitcoin/bitcoin#18195 Add `cost_of_change` parameter assertions to `bnb_search_test` (yancyribbens)
- bitcoin/bitcoin#18209 Reduce unneeded whitelist permissions in tests (MarcoFalke)
- bitcoin/bitcoin#18211 Disable mockforward scheduler unit test for now (MarcoFalke)
- bitcoin/bitcoin#18213 Fix race in `p2p_segwit` (MarcoFalke)
- bitcoin/bitcoin#18224 Make AnalyzePSBT next role calculation simple, correct (instagibbs)
- bitcoin/bitcoin#18228 Add missing syncwithvalidationinterfacequeue (MarcoFalke)
- bitcoin/bitcoin#18247 Wait for both veracks in `add_p2p_connection` (MarcoFalke)
- bitcoin/bitcoin#18249 Bump timeouts to accomodate really slow disks (MarcoFalke)
- bitcoin/bitcoin#18255 Add `bad-txns-*-toolarge` test cases to `invalid_txs` (MarcoFalke)
- bitcoin/bitcoin#18263 rpc: change setmocktime check to use IsMockableChain (gzhao408)
- bitcoin/bitcoin#18285 Check that `wait_until` returns if time point is in the past (MarcoFalke)
- bitcoin/bitcoin#18286 Add locale fuzzer to `FUZZERS_MISSING_CORPORA` (practicalswift)
- bitcoin/bitcoin#18292 fuzz: Add `assert(script == decompressed_script)` (MarcoFalke)
- bitcoin/bitcoin#18299 Update `FUZZERS_MISSING_CORPORA` to enable regression fuzzing for all harnesses in master (practicalswift)
- bitcoin/bitcoin#18300 fuzz: Add option to merge input dir to test runner (MarcoFalke)
- bitcoin/bitcoin#18305 Explain why test logging should be used (MarcoFalke)
- bitcoin/bitcoin#18306 Add logging to `wallet_listsinceblock.py` (jonatack)
- bitcoin/bitcoin#18311 Bumpfee test fix (instagibbs)
- bitcoin/bitcoin#18314 Add deserialization fuzzing of SnapshotMetadata (`utxo_snapshot`) (practicalswift)
- bitcoin/bitcoin#18319 fuzz: Add missing `ECC_Start` to `key_io` test (MarcoFalke)
- bitcoin/bitcoin#18334 Add basic test for BIP 37 (MarcoFalke)
- bitcoin/bitcoin#18350 Fix mining to an invalid target + ensure that a new block has the correct hash internally (TheQuantumPhysicist)
- bitcoin/bitcoin#18378 Bugfix & simplify bn2vch using `int.to_bytes` (sipa)
- bitcoin/bitcoin#18393 Don't assume presence of `__builtin_mul_overflow(…)` in `MultiplicationOverflow(…)` fuzzing harness (practicalswift)
- bitcoin/bitcoin#18406 add executable flag for `rpc_estimatefee.py` (theStack)
- bitcoin/bitcoin#18420 listsinceblock block height checks (jonatack)
- bitcoin/bitcoin#18430 ci: Only clone bitcoin-core/qa-assets when fuzzing (MarcoFalke)
- bitcoin/bitcoin#18438 ci: Use homebrew addon on native macos (hebasto)
- bitcoin/bitcoin#18447 Add coverage for script parse error in ParseScript (pierreN)
- bitcoin/bitcoin#18472 Remove unsafe `BOOST_TEST_MESSAGE` (MarcoFalke)
- bitcoin/bitcoin#18474 check that peer is connected when calling sync_* (MarcoFalke)
- bitcoin/bitcoin#18477 ci: Use focal for fuzzers (MarcoFalke)
- bitcoin/bitcoin#18481 add BIP37 'filterclear' test to p2p_filter.py (theStack)
- bitcoin/bitcoin#18496 Remove redundant `sync_with_ping` after `add_p2p_connection` (jonatack)
- bitcoin/bitcoin#18509 fuzz: Avoid running over all inputs after merging them (MarcoFalke)
- bitcoin/bitcoin#18510 fuzz: Add CScriptNum::getint coverage (MarcoFalke)
- bitcoin/bitcoin#18514 remove rapidcheck integration and tests (fanquake)
- bitcoin/bitcoin#18515 Add BIP37 remote crash bug [CVE-2013-5700] test to `p2p_filter.py` (theStack)
- bitcoin/bitcoin#18516 relax bumpfee `dust_to_fee` txsize an extra vbyte (jonatack)
- bitcoin/bitcoin#18518 fuzz: Extend descriptor fuzz test (MarcoFalke)
- bitcoin/bitcoin#18519 fuzz: Extend script fuzz test (MarcoFalke)
- bitcoin/bitcoin#18521 fuzz: Add `process_messages` harness (MarcoFalke)
- bitcoin/bitcoin#18529 Add fuzzer version of randomized prevector test (sipa)
- bitcoin/bitcoin#18534 skip backwards compat tests if not compiled with wallet (fanquake)
- bitcoin/bitcoin#18540 `wallet_bumpfee` assertion fixup (jonatack)
- bitcoin/bitcoin#18543 Use one node to avoid a race due to missing sync in `rpc_signrawtransaction` (MarcoFalke)
- bitcoin/bitcoin#18561 Properly raise FailedToStartError when rpc shutdown before warmup finished (MarcoFalke)
- bitcoin/bitcoin#18562 ci: Run unit tests sequential once (MarcoFalke)
- bitcoin/bitcoin#18563 Fix `unregister_all_during_call` cleanup (ryanofsky)
- bitcoin/bitcoin#18566 Set `-use_value_profile=1` when merging fuzz inputs (MarcoFalke)
- bitcoin/bitcoin#18757 Remove enumeration of expected deserialization exceptions in ProcessMessage(…) fuzzer (practicalswift)
- bitcoin/bitcoin#18878 Add test for conflicted wallet tx notifications (ryanofsky)
- bitcoin/bitcoin#18975 Remove const to work around compiler error on xenial (laanwj)
- bitcoin/bitcoin#19444 Remove cached directories and associated script blocks from appveyor config (sipsorcery)
- bitcoin/bitcoin#18640 appveyor: Remove clcache (MarcoFalke)
- bitcoin/bitcoin#19839 Set appveyor vm version to previous Visual Studio 2019 release. (sipsorcery)
- bitcoin/bitcoin#19842 Update the vcpkg checkout commit ID in appveyor config. (sipsorcery)
- bitcoin/bitcoin#20562 Test that a fully signed tx given to signrawtx is unchanged (achow101)

### Documentation
- bitcoin/bitcoin#16947 Doxygen-friendly script/descriptor.h comments (ch4ot1c)
- bitcoin/bitcoin#16983 Add detailed info about Bitcoin Core files (hebasto)
- bitcoin/bitcoin#16986 Doxygen-friendly CuckooCache comments (ch4ot1c)
- bitcoin/bitcoin#17022 move-only: Steps for "before major release branch-off" (MarcoFalke)
- bitcoin/bitcoin#17026 Update bips.md for default bech32 addresses in 0.20.0 (MarcoFalke)
- bitcoin/bitcoin#17081 Fix Makefile target in benchmarking.md (theStack)
- bitcoin/bitcoin#17102 Add missing indexes/blockfilter/basic to doc/files.md (MarcoFalke)
- bitcoin/bitcoin#17119 Fix broken bitcoin-cli examples (andrewtoth)
- bitcoin/bitcoin#17134 Add switch on enum example to developer notes (hebasto)
- bitcoin/bitcoin#17142 Update macdeploy README to include all files produced by `make deploy` (za-kk)
- bitcoin/bitcoin#17146 github: Add warning for bug reports (laanwj)
- bitcoin/bitcoin#17157 Added instructions for how to add an upsteam to forked repo (dannmat)
- bitcoin/bitcoin#17159 Add a note about backporting (carnhofdaki)
- bitcoin/bitcoin#17169 Correct function name in ReportHardwareRand() (fanquake)
- bitcoin/bitcoin#17177 Describe log files + consistent paths in test READMEs (fjahr)
- bitcoin/bitcoin#17239 Changed miniupnp links to https (sandakersmann)
- bitcoin/bitcoin#17281 Add developer note on `c_str()` (laanwj)
- bitcoin/bitcoin#17285 Bip70 removal follow-up (fjahr)
- bitcoin/bitcoin#17286 Fix help-debug -checkpoints (ariard)
- bitcoin/bitcoin#17309 update MSVC instructions to remove Qt OpenSSL linking (fanquake)
- bitcoin/bitcoin#17339 Add template for good first issues (michaelfolkson)
- bitcoin/bitcoin#17351 Fix some misspellings (RandyMcMillan)
- bitcoin/bitcoin#17353 Add ShellCheck to lint tests dependencies (hebasto)
- bitcoin/bitcoin#17370 Update doc/bips.md with recent changes in master (MarcoFalke)
- bitcoin/bitcoin#17393 Added regtest config for linearize script (gr0kchain)
- bitcoin/bitcoin#17411 Add some better examples for scripted diff (laanwj)
- bitcoin/bitcoin#17503 Remove bitness from bitcoin-qt help message and manpage (laanwj)
- bitcoin/bitcoin#17539 Update and improve Developer Notes (hebasto)
- bitcoin/bitcoin#17561 Changed MiniUPnPc link to https in dependencies.md (sandakersmann)
- bitcoin/bitcoin#17596 Change doxygen URL to doxygen.bitcoincore.org (laanwj)
- bitcoin/bitcoin#17598 Update release process with latest changes (MarcoFalke)
- bitcoin/bitcoin#17617 Unify unix epoch time descriptions (jonatack)
- bitcoin/bitcoin#17637 script: Add keyserver to verify-commits readme (emilengler)
- bitcoin/bitcoin#17648 Rename wallet-tool references to bitcoin-wallet (hel-o)
- bitcoin/bitcoin#17688 Add "ci" prefix to CONTRIBUTING.md (hebasto)
- bitcoin/bitcoin#17751 Use recommended shebang approach in documentation code block (hackerrdave)
- bitcoin/bitcoin#17752 Fix directory path for secp256k1 subtree in developer-notes (hackerrdave)
- bitcoin/bitcoin#17772 Mention PR Club in CONTRIBUTING.md (emilengler)
- bitcoin/bitcoin#17804 Misc RPC help fixes (MarcoFalke)
- bitcoin/bitcoin#17819 Developer notes guideline on RPCExamples addresses (jonatack)
- bitcoin/bitcoin#17825 Update dependencies.md (hebasto)
- bitcoin/bitcoin#17873 Add to Doxygen documentation guidelines (jonatack)
- bitcoin/bitcoin#17907 Fix improper Doxygen inline comments (Empact)
- bitcoin/bitcoin#17942 Improve fuzzing docs for macOS users (fjahr)
- bitcoin/bitcoin#17945 Fix doxygen errors (Empact)
- bitcoin/bitcoin#18025 Add missing supported rpcs to doc/descriptors.md (andrewtoth)
- bitcoin/bitcoin#18070 Add note about `brew doctor` (givanse)
- bitcoin/bitcoin#18125 Remove PPA note from release-process.md (fanquake)
- bitcoin/bitcoin#18170 Minor grammatical changes and flow improvements (travinkeith)
- bitcoin/bitcoin#18212 Add missing step in win deployment instructions (dangershony)
- bitcoin/bitcoin#18219 Add warning against wallet.dat re-use (corollari)
- bitcoin/bitcoin#18253 Correct spelling errors in comments (Empact)
- bitcoin/bitcoin#18278 interfaces: Describe and follow some code conventions (ryanofsky)
- bitcoin/bitcoin#18283 Explain rebase policy in CONTRIBUTING.md (MarcoFalke)
- bitcoin/bitcoin#18340 Mention MAKE=gmake workaround when building on a BSD (fanquake)
- bitcoin/bitcoin#18341 Replace remaining literal BTC with `CURRENCY_UNIT` (domob1812)
- bitcoin/bitcoin#18342 Add fuzzing quickstart guides for libFuzzer and afl-fuzz (practicalswift)
- bitcoin/bitcoin#18344 Fix nit in getblockchaininfo (stevenroose)
- bitcoin/bitcoin#18379 Comment fix merkle.cpp (4d55397500)
- bitcoin/bitcoin#18382 note the costs of fetching all pull requests (vasild)
- bitcoin/bitcoin#18391 Update init and reduce-traffic docs for -blocksonly (glowang)
- bitcoin/bitcoin#18464 Block-relay-only vs blocksonly (MarcoFalke)
- bitcoin/bitcoin#18486 Explain new test logging (MarcoFalke)
- bitcoin/bitcoin#18505 Update webchat URLs in README.md (SuriyaaKudoIsc)
- bitcoin/bitcoin#18513 Fix git add argument (HashUnlimited)
- bitcoin/bitcoin#18577 Correct scripted-diff example link (yahiheb)
- bitcoin/bitcoin#18589 Fix naming of macOS SDK and clarify version (achow101)

### Miscellaneous
- bitcoin/bitcoin#15600 lockedpool: When possible, use madvise to avoid including sensitive information in core dumps (luke-jr)
- bitcoin/bitcoin#15934 Merge settings one place instead of five places (ryanofsky)
- bitcoin/bitcoin#16115 On bitcoind startup, write config args to debug.log (LarryRuane)
- bitcoin/bitcoin#16117 util: Replace boost sleep with std sleep (MarcoFalke)
- bitcoin/bitcoin#16161 util: Fix compilation errors in support/lockedpool.cpp (jkczyz)
- bitcoin/bitcoin#16802 scripts: In linearize, search for next position of magic bytes rather than fail (takinbo)
- bitcoin/bitcoin#16889 Add some general std::vector utility functions (sipa)
- bitcoin/bitcoin#17049 contrib: Bump gitian descriptors for 0.20 (MarcoFalke)
- bitcoin/bitcoin#17052 scripts: Update `copyright_header` script to include additional files (GChuf)
- bitcoin/bitcoin#17059 util: Simplify path argument for cblocktreedb ctor (hebasto)
- bitcoin/bitcoin#17191 random: Remove call to `RAND_screen()` (Windows only) (fanquake)
- bitcoin/bitcoin#17192 util: Add `check_nonfatal` and use it in src/rpc (MarcoFalke)
- bitcoin/bitcoin#17218 Replace the LogPrint function with a macro (jkczyz)
- bitcoin/bitcoin#17266 util: Rename decodedumptime to parseiso8601datetime (elichai)
- bitcoin/bitcoin#17270 Feed environment data into RNG initializers (sipa)
- bitcoin/bitcoin#17282 contrib: Remove accounts from bash completion (fanquake)
- bitcoin/bitcoin#17293 Add assertion to randrange that input is not 0 (JeremyRubin)
- bitcoin/bitcoin#17325 log: Fix log message for -par=1 (hebasto)
- bitcoin/bitcoin#17329 linter: Strip trailing / in path for git-subtree-check (jnewbery)
- bitcoin/bitcoin#17336 scripts: Search for first block file for linearize-data with some block files pruned (Rjected)
- bitcoin/bitcoin#17361 scripts: Lint gitian descriptors with shellcheck (hebasto)
- bitcoin/bitcoin#17482 util: Disallow network-qualified command line options (ryanofsky)
- bitcoin/bitcoin#17507 random: mark RandAddPeriodic and SeedPeriodic as noexcept (fanquake)
- bitcoin/bitcoin#17527 Fix CPUID subleaf iteration (sipa)
- bitcoin/bitcoin#17604 util: Make schedulebatchpriority advisory only (fanquake)
- bitcoin/bitcoin#17650 util: Remove unwanted fields from bitcoin-cli -getinfo (malevolent)
- bitcoin/bitcoin#17671 script: Fixed wget call in gitian-build.py (willyko)
- bitcoin/bitcoin#17699 Make env data logging optional (sipa)
- bitcoin/bitcoin#17721 util: Don't allow base58 decoding of non-base58 strings. add base58 tests (practicalswift)
- bitcoin/bitcoin#17750 util: Change getwarnings parameter to bool (jnewbery)
- bitcoin/bitcoin#17753 util: Don't allow base32/64-decoding or parsemoney(…) on strings with embedded nul characters. add tests (practicalswift)
- bitcoin/bitcoin#17823 scripts: Read suspicious hosts from a file instead of hardcoding (sanjaykdragon)
- bitcoin/bitcoin#18162 util: Avoid potential uninitialized read in `formatiso8601datetime(int64_t)` by checking `gmtime_s`/`gmtime_r` return value (practicalswift)
- bitcoin/bitcoin#18167 Fix a violation of C++ standard rules where unions are used for type-punning (TheQuantumPhysicist)
- bitcoin/bitcoin#18225 util: Fail to parse empty string in parsemoney (MarcoFalke)
- bitcoin/bitcoin#18270 util: Fail to parse whitespace-only strings in parsemoney(…) (instead of parsing as zero) (practicalswift)
- bitcoin/bitcoin#18316 util: Helpexamplerpc formatting (jonatack)
- bitcoin/bitcoin#18357 Fix missing header in sync.h (promag)
- bitcoin/bitcoin#18412 script: Fix `script_err_sig_pushonly` error string (theStack)
- bitcoin/bitcoin#18416 util: Limit decimal range of numbers parsescript accepts (pierreN)
- bitcoin/bitcoin#18503 init: Replace `URL_WEBSITE` with `PACKAGE_URL` (MarcoFalke)
- bitcoin/bitcoin#18526 Remove PID file at the very end (hebasto)
- bitcoin/bitcoin#18553 Avoid non-trivial global constants in SHA-NI code (sipa)
- bitcoin/bitcoin#18665 Do not expose and consider `-logthreadnames` when it does not work (hebasto)
- bitcoin/bitcoin#19194 util: Don't reference errno when pthread fails (miztake)
- bitcoin/bitcoin#18700 Fix locking on WSL using flock instead of fcntl (meshcollider)
- bitcoin/bitcoin#19192 Extract net permissions doc (MarcoFalke)
- bitcoin/bitcoin#19777 Correct description for getblockstats’s txs field (shesek)
- bitcoin/bitcoin#20080 Strip any trailing / in -datadir and -blocksdir paths (hebasto)
- bitcoin/bitcoin#20082 fixes read buffer to use min rather than max (EthanHeilman)
- bitcoin/bitcoin#20141 Avoid the use of abs64 in timedata (sipa)
- bitcoin/bitcoin#20756 Add missing field (permissions) to the getpeerinfo help (amitiuttarwar)
- bitcoin/bitcoin#20861 BIP 350: Implement Bech32m and use it for v1+ segwit addresses (sipa)
- bitcoin/bitcoin#22124 Update translations after closing 0.20.x on Transifex (hebasto)
- bitcoin/bitcoin#21471 fix bech32_encode calls in gen_key_io_test_vectors.py (sipa)
- bitcoin/bitcoin#22837 mention bech32m/BIP350 in doc/descriptors.md (sipa)

Credits
-------

Thanks to everyone who directly contributed to this release:

- 0xb10c
- 251
- 4d55397500
- Aaron Clauson
- Adam Jonas
- Albert
- Amiti Uttarwar
- Andrew Chow
- Andrew Toth
- Anthony Towns
- Antoine Riard
- Ava Barron
- Ben Carman
- Ben Woosley
- Block Mechanic
- Brian Solon
- Bushstar
- Carl Dong
- Carnhof Daki
- Cory Fields
- Daki Carnhof
- Dan Gershony
- Daniel Kraft
- dannmat
- Danny-Scott
- darosior
- David O'Callaghan
- Dominik Spicher
- Elichai Turkel
- Emil Engler
- emu
- Ethan Heilman
- Fabian Jahr
- fanquake
- Filip Gospodinov
- Franck Royer
- Gastón I. Silva
- gchuf
- Gleb Naumenko
- Gloria Zhao
- glowang
- Gr0kchain
- Gregory Sanders
- hackerrdave
- Harris
- hel0
- Hennadii Stepanov
- ianliu
- Igor Cota
- James Chiang
- James O'Beirne
- Jan Beich
- Jan Sarenik
- Jeffrey Czyz
- Jeremy Rubin
- JeremyCrookshank
- Jim Posen
- João Barbosa
- John Bampton
- John L. Jegutanis
- John Newbery
- Jon Atack
- Jon Layton
- Jonas Schnelli
- Jorge Timón
- Karl-Johan Alm
- kodslav
- Larry Ruane
- Luke Dashjr
- malevolent
- MapleLaker
- marcaiaf
- MarcoFalke
- Marius Kjærstad
- Mark Erhardt
- Mark Friedenbach
- Mark Tyneway
- Martin Erlandsson
- Martin Zumsande
- Matt Corallo
- Matt Ward
- Michael Folkson
- Michael Polzer
- Micky Yun Chan
- MIZUTA Takeshi
- Nadav Ivgi
- naumenkogs
- Neha Narula
- nijynot
- NullFunctor
- Peter Bushnell
- pierrenn
- Pieter Wuille
- practicalswift
- randymcmillan
- Rjected
- Russell Yanofsky
- sachinkm77
- Samer Afach
- Samuel Dobson
- Sanjay K
- Sebastian Falbesoner
- setpill
- Sjors Provoost
- Stefan Richter
- stefanwouldgo
- Steven Roose
- Suhas Daftuar
- Suriyaa Sundararuban
- TheCharlatan
- Tim Akinbo
- Travin Keith
- tryphe
- Vasil Dimov
- Willy Ko
- Wilson Ccasihue S
- Wladimir J. van der Laan
- Yahia Chiheb
- Yancy Ribbens
- Yusuf Sahin HAMZA
- Zakk
- Zero

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
