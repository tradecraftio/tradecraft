v23.1-33164 Release Notes
=========================

Freicoin version v23.1-33164 is now available from:

  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-aarch64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, big endian)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-powerpc64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, little endian)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-powerpc64le-linux-gnu.tar.gz)
  * [Linux RISC-V 64-bit (RV64GC)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-riscv64-linux-gnu.tar.gz)
  * [macOS (Apple Silicon, app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-arm64-apple-darwin.dmg)
  * [macOS (Apple Silicon, server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-arm64-apple-darwin.tar.gz)
  * [macOS (Intel, app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-x86_64-apple-darwin.dmg)
  * [macOS (Intel, server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-x86_64-apple-darwin.tar.gz)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v23.1-33164-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v23.1-33164.zip)

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tradecraftio/tradecraft/issues>

To receive security and update notifications, please subscribe to:

  <https://tradecraft.groups.io/g/announce/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the installer
(on Windows) or just copy over `/Applications/Freicoin-Qt` (on Mac) or
`freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
==============

Freicoin is supported and extensively tested on operating systems using the
Linux kernel, macOS 10.15+, and Windows 7 and newer.  Freicoin should also work
on most other Unix-like systems but is not as frequently tested on them.  It is
not recommended to use Freicoin on unsupported systems.

Notable changes
===============

P2P and network changes
-----------------------

- A freicoind node will no longer rumour addresses to inbound peers by default.
  They will become eligible for address gossip after sending an ADDR, ADDRV2, or
  GETADDR message.  (bitcoin#21528)

- Before this release, Freicoin had a strong preference to try to connect only
  to peers that listen on port 8639.  As a result of that, Freicoin nodes
  listening on non-standard ports would likely not get any Freicoin peers
  connecting to them.  This preference has been removed.  (bitcoin#23542)

- Full support has been added for the CJDNS network.  See the new option
  `-cjdnsreachable` and
  [doc/cjdns.md](https://github.com/tradecraftio/tradecraft/tree/23/doc/cjdns.md).
  (bitcoin#23077)

Fee estimation changes
----------------------

- Fee estimation now takes the feerate of replacement (RBF) transactions into
  account.  (bitcoin#22539)

Rescan startup parameter removed
--------------------------------

The `-rescan` startup parameter has been removed. Wallets which require
rescanning due to corruption will still be rescanned on startup.  Otherwise,
please use the `rescanblockchain` RPC to trigger a rescan.  (bitcoin#23123)

Tracepoints and Userspace, Statically Defined Tracing support
-------------------------------------------------------------

Freicoin release binaries for Linux now include experimental tracepoints which
act as an interface for process-internal events.  These can be used for review,
debugging, monitoring, and more.  The tracepoint API is semi-stable.  While the
API is tested, process internals might change between releases requiring changes
to the tracepoints.  Information about the existing tracepoints can be found
under
[doc/tracing.md](https://github.com/tradecraftio/tradecraft/blob/23/doc/tracing.md)
and usage examples are provided in
[contrib/tracing/](https://github.com/tradecraftio/tradecraft/tree/23/contrib/tracing).

Updated RPCs
------------

- The `validateaddress` RPC now returns an `error_locations` array for invalid
  addresses, with the indices of invalid character locations in the address (if
  known).  For example, this will attempt to locate up to two Bech32 errors, and
  return their locations if successful.  Success and correctness are only
  guaranteed if fewer than two substitution errors have been made.  The error
  message returned in the `error` field now also returns more specific errors
  when decoding fails.  (bitcoin#16807)

- The `-deprecatedrpc=addresses` configuration option has been removed.  RPCs
  `gettxout`, `getrawtransaction`, `decoderawtransaction`, `decodescript`,
  `gettransaction verbose=true` and REST endpoints `/rest/tx`, `/rest/getutxos`,
  `/rest/block` no longer return the `addresses` and `reqSigs` fields, which
  were previously deprecated in v22.  (bitcoin#22650)

- The `getblock` RPC command now supports verbosity level 3 containing
  transaction inputs' `prevout` information.  The existing `/rest/block/` REST
  endpoint is modified to contain this information too.  Every `vin` field will
  contain an additional `prevout` subfield describing the spent output.
  `prevout` contains the following keys:
    - `generated` - true if the spent coins was a coinbase.
    - `height`
    - `value`
    - `scriptPubKey`

- The top-level fee fields `fee`, `modifiedfee`, `ancestorfees` and
  `descendantfees` returned by RPCs `getmempoolentry`,
  `getrawmempool(verbose=true)`, `getmempoolancestors(verbose=true)` and
  `getmempooldescendants(verbose=true)` are deprecated and will be removed in
  the next major version (use `-deprecated=fees` if needed in this version).
  The same fee fields can be accessed through the `fees` object in the result.
  WARNING: deprecated fields `ancestorfees` and `descendantfees` are denominated
  in kria, whereas all fields in the `fees` object are denominated in FRC.
  (bitcoin#22689)

- Both `createmultisig` and `addmultisigaddress` now include a `warnings` field,
  which will show a warning if a non-legacy address type is requested when using
  uncompressed public keys.  (bitcoin#23113)

Changes to wallet related RPCs can be found in the Wallet section below.

New RPCs
--------

- Information on soft fork status has been moved from `getblockchaininfo` to the
  new `getdeploymentinfo` RPC which allows querying soft fork status at any
  block, rather than just at the chain tip.  Inclusion of soft fork status in
  `getblockchaininfo` can currently be restored using the configuration
  `-deprecatedrpc=softforks`, but this will be removed in a future release.
  Note that in either case, the `status` field now reflects the status of the
  current block rather than the next block.  (bitcoin#23508)

Files
-----

- On startup, the list of banned hosts and networks (via `setban` RPC) in
  `banlist.dat` is ignored and only `banlist.json` is considered.  Freicoin
  version v22 is the only version that can read `banlist.dat` and also write it
  to `banlist.json`.  If `banlist.json` already exists, version v22 will not try
  to translate the `banlist.dat` into json.  After an upgrade, `listbanned` can
  be used to double check the parsed entries.  (bitcoin#22570)

Updated settings
----------------

- In previous releases, the meaning of the command line option `-persistmempool`
  (without a value provided) incorrectly disabled mempool persistence.
  `-persistmempool` is now treated like other boolean options to mean
  `-persistmempool=1`.  Passing `-persistmempool=0`, `-persistmempool=1` and
  `-nopersistmempool` is unaffected.  (bitcoin#23061)

- `-maxuploadtarget` now allows human readable byte units [k|K|m|M|g|G|t|T].
  E.g. `-maxuploadtarget=500g`.  No whitespace, +- or fractions allowed.
  Default is `M` if no suffix provided.  (bitcoin#23249)

- If `-proxy=` is given together with `-noonion` then the provided proxy will
  not be set as a proxy for reaching the Tor network.  So it will not be
  possible to open manual connections to the Tor network for example with the
  `addnode` RPC.  To mimic the old behavior use `-proxy=` together with
  `-onlynet=` listing all relevant networks except `onion`.  (bitcoin#22834)

Tools and Utilities
-------------------

- Update `-getinfo` to return data in a user-friendly format that also reduces
  vertical space.  (bitcoin#21832)

- CLI `-addrinfo` now returns a single field for the number of `onion` addresses
  known to the node instead of separate `torv2` and `torv3` fields, as support
  for Tor V2 addresses was removed from Freicoin in v22.  (bitcoin#22544)

Wallet
------

- Descriptor wallets are now the default wallet type. Newly created wallets will
  use descriptors unless `descriptors=false` is set during `createwallet`, or
  the `Descriptor wallet` checkbox is unchecked in the GUI.

  Note that wallet RPC commands like `importmulti` and `dumpprivkey` cannot be
  used with descriptor wallets, so if your client code relies on these commands
  without specifying `descriptors=false` during wallet creation, you will need
  to update your code.

- `upgradewallet` will now automatically flush the keypool if upgrading from a
  non-HD wallet to an HD wallet, to immediately start using the newly-generated
  HD keys.  (bitcoin#23093)

- a new RPC `newkeypool` has been added, which will flush (entirely clear and
  refill) the keypool.  (bitcoin#23093)

- `listunspent` now includes `ancestorcount`, `ancestorsize`, and `ancestorfees`
  for each transaction output that is still in the mempool.  (bitcoin#12677)

- `lockunspent` now optionally takes a third parameter, `persistent`, which
  causes the lock to be written persistently to the wallet database.  This
  allows UTXOs to remain locked even after node restarts or crashes.
  (bitcoin#23065)

- `receivedby` RPCs now include coinbase transactions.  Previously, the
  following wallet RPCs excluded coinbase transactions: `getreceivedbyaddress`,
  `getreceivedbylabel`, `listreceivedbyaddress`, `listreceivedbylabel`.  This
  release changes this behaviour and returns results accounting for received
  coins from coinbase outputs.  The previous behaviour can be restored using the
  configuration `-deprecatedrpc=exclude_coinbase`, but may be removed in a
  future release.  (bitcoin#14707)

- A new option in the same `receivedby` RPCs, `include_immature_coinbase`
  (default=`false`), determines whether to account for immature coinbase
  transactions.  Immature coinbase transactions are coinbase transactions that
  have 100 or fewer confirmations, and are not spendable.  (bitcoin#14707)

GUI changes
-----------

- UTXOs which are locked via the GUI are now stored persistently in the wallet
  database, so are not lost on node shutdown or crash.  (bitcoin#23065)

- The Bech32 checkbox has been replaced with a dropdown for all address types.

Low-level changes
=================

RPC
---

- `getblockchaininfo` now returns a new `time` field, that provides the chain
  tip time.  (bitcoin#22407)

Tests
-----

- For the `regtest` network the activation heights of several softforks were set
  to block height 1.  They can be changed by the runtime setting
  `-testactivationheight=name@height`.  (bitcoin#22818)

Credits
=======

Thanks to everyone who directly contributed to this release:

- 0xb10c
- 0xree
- Aaron Clauson
- Adrian-Stefan Mares
- agroce
- aitorjs
- Alex Groce
- amadeuszpawlik
- Amiti Uttarwar
- Andrew Chow
- Andrew Chow
- Andrew Poelstra
- Andrew Toth
- anouar kappitou
- Anthony Towns
- Antoine Poinsot
- Arnab Sen
- Ben Woosley
- benthecarman
- Bitcoin Hodler
- BitcoinTsunami
- brianddk
- Bruno Garcia
- brunoerg
- CallMeMisterOwl
- Calvin Kim
- Carl Dong
- Cory Fields
- Cuong V. Nguyen
- Darius Parvin
- Dhruv Mehta
- Dimitri Deijs
- Dimitris Apostolou
- Dmitry Goncharov
- Douglas Chimento
- eugene
- Fabian Jahr
- fanquake
- Florian Baumgartl
- fyquah
- Gleb Naumenko
- glozow
- Gregory Sanders
- Heebs
- Hennadii Stepanov
- Hennadii Stepanov
- hg333
- HiLivin
- Igor Cota
- Jadi
- James O'Beirne
- Jameson Lopp
- Jarol Rodriguez
- Jeremy Rand
- Jeremy Rubin
- Joan Karadimov
- João Barbosa
- John Moffett
- John Newbery
- Jon Atack
- josibake
- junderw
- Karl-Johan Alm
- katesalazar
- Kennan Mell
- Kiminuo
- Kittywhiskers Van Gogh
- Klement Tan
- Kristaps Kaupe
- Kuro
- Larry Ruane
- lsilva01
- lucash-dev
- Luke Dashjr
- MacroFake
- MarcoFalke
- Mark Friedenbach
- Martin Leitner-Ankerl
- Martin Zumsande
- Martin Zumsande
- Matt Corallo
- Matt Whitlock
- MeshCollider
- Michael Dietz
- Michael Ford
- Murch
- muxator
- naiza
- Nathan Garabedian
- Nelson Galdeman
- NikhilBartwal
- Niklas Gögge
- node01
- nthumann
- Pasta
- Patrick Kamin
- Pavel Safronov
- Pavol Rusnak
- Pavol Rusnak
- Perlover
- Pieter Wuille
- practicalswift
- pradumnasaraf
- pranabp-bit
- Prateek Sancheti
- Prayank
- Rafael Sadowski
- rajarshimaitra
- randymcmillan
- ritickgoenka
- Rob Fielding
- Rojar Smith
- Russell Yanofsky
- S3RK
- Saibato
- Samuel Dobson
- sanket1729
- seaona
- Sebastian Falbesoner
- Sebastian Falbesoner
- sh15h4nk
- Shashwat
- Shorya
- ShubhamPalriwala
- Shubhankar Gambhir
- Sjors Provoost
- sogoagain
- sstone
- stratospher
- Suriyaa Rocky Sundararuban
- Taeik Lim
- TheCharlatan
- Tim Ruffing
- Tobin Harding
- Troy Giorshev
- Tyler Chambers
- Vasil Dimov
- W. J. van der Laan
- W. J. van der Laan
- w0xlt
- willcl-ark
- William Casarin
- zealsham
- Zero-1729

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
