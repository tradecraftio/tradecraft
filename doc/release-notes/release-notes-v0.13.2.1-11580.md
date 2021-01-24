Bitcoin version 0.13.2.1-11580 is now available from:

  * [Linux i686 (Intel 32-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-i686-pc-linux-gnu.tar.gz)
  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-aarch64-linux-gnu.tar.gz)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v0.13.2.1-11580-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/bitcoin-v0.13.2.1-11580.zip)

This is the first release of the v0.13 stable branch of Bitcoin Core with the
stratum mining server and Tradecraft/Freicoin merge-mining patches applied.

Please report bugs related to the stratum mining server implementation or
Tradecraft/Freicoin merge-mining at the Tradecraft issue tracker on GitHub:

  <https://github.com/tradecraftio/tradecraft/issues>

Please report other bugs using Bitcoin the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications for the stratum mining server and
merge-mining patches, please subscribe to:

  <https://tradecraft.groups.io/g/announce/>

To receive security notifications for Bitcoin Core, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

Compatibility
==============

Microsoft ended support for Windows XP on [April 8th,
2014](https://www.microsoft.com/en-us/WindowsForBusiness/end-of-xp-support), an
OS initially released in 2001. This means that not even critical security
updates will be released anymore. Without security updates, using a bitcoin
wallet on a XP machine is irresponsible at least.

In addition to that, with 0.12.x there have been varied reports of Bitcoin Core
randomly crashing on Windows XP. It is [not
clear](https://github.com/bitcoin/bitcoin/issues/7681#issuecomment-217439891)
what the source of these crashes is, but it is likely that upstream libraries
such as Qt are no longer being tested on XP.

We do not have time nor resources to provide support for an OS that is
end-of-life. From 0.13.0 on, Windows XP is no longer supported. Users are
suggested to upgrade to a newer version of Windows, or install an alternative OS
that is supported.

No attempt is made to prevent installing or running the software on Windows XP,
you can still do so at your own risk, but do not expect it to work: do not
report issues about Windows XP to the issue tracker.

From 0.13.1 onwards OS X 10.7 is no longer supported. 0.13.0 was intended to
work on 10.7+, but severe issues with the libc++ version on 10.7.x keep it from
running reliably.  0.13.1 now requires 10.8+, and will communicate that to 10.7
users, rather than crashing unexpectedly.

Notable changes
===============

Please see the upstream release notes for the version of Bitcoin Core upon which
this release was based:

  <https://bitcoin.org/en/release/v0.13.2>

Below are the notable changes specific to this stratum mining server and
Tradecraft/Freicoin merge-mining fork of Bitcoin Core.

Introduce built-in stratum mining server with version-rolling support
---------------------------------------------------------------------

This release includes, for the first time, a built-in stratum mining server for
solo-mining clients.  This service is established on port 9332 for main net, but
can be configured with the `-stratumport` option.  For security reasons the
stratum server only accepts connections from the loopback interface, but this
default can be overridden with the `-stratumallowip` and `-stratumbind` options.

It is anticipated that a future release will introduce a decentralized mining
pool allowing compensation for partial proof-of-work solutions.  However at this
time the stratum server only provides solo-mining work, with the entire payout
of each block assigned to the miner that solves it.

To use, configure your miner to connect to the interface and port used,
e.g. `stratum+tcp://127.0.0.1:9332`, and provide as the username the freicoin
address you desire to collect the mining proceeds.  The username may optionally
be suffixed with '+' followed by a numeric minimum difficulty.  The password is
used for merge-mining support (see below).  If merge-mining is not used, it may
take any value.

The stratum server supports the version-rolling extension for compatibility with
mining devices implementing ASCIBoost optimizations.

Tradecraft/Freicoin merge-mining
--------------------------------

The built-in stratum mining server supports merge-mining of subordinate chains
which share bitcoin's proof-of-work algorithm, using the merge-mining scheme
developed by the Tradecraft project for the Freicoin block chain.

To enable merge-mining, configure the '-mergemine' option to specify the IP and
port of the subordinate chain's stratum mining server, for example
'-mergemine=127.0.0.1:9638'.  This option can be specified multiple times to
support merge-mining for multiple chains.

Next configure miners to connect with the subordinate chains' mining addresses
in the stratum password.  Each subordinate chain has its own chain-specific ID,
a 256-bit hex-encoded string.  Set the miner's stratum password to a comma-
separated list of 'chainid=subaddress' pairs, where 'chainid' is the chain ID
string and 'subaddress' is the miner's coinbase payout address for that chain.

As a convenience, the bitcoin stratum mining server may be configured to accept
a common name in lieu of the 256-bit hex string chain identifier.  The following
built-in common names are recognized:

  "tradecraft"/"freicoin": Either string identifies the Freicoin main-net block
    chain, f67ff506d759ce02e47862d6aafbcec5c636b3169add3cf6b7632e75ec382963

  "testnet": Identifies the Tradecraft/Freicoin test-net block chain,
    8bb1d12a1a0f7099b4d5283935316d5d388aff4558138de2f7e2cefa4bc49fe9

  "regtest": Identifies the Tradecraft/Freicoin regression test block chain,
    74fcf0055e226f5c2f4eb850c26ebb32746a04ba93776a7ec71a1cf01ad499d7

Additional common name identifiers can be registered with the
'-mergeminename=name:chainid' option.

Merge-mining single-chain limitation
------------------------------------

As of this time, the merge-mining implementation is limited to only merge-mining
one subordinate chain at a time.  It is expected that future releases will
support simultaneous merge-mining of multiple subordinate chains.

v0.13.2.1-11580 Change log
==========================

The following pull requests were merged into the Tradecraft code repository for
this release.

- #91 Add stratum server and merge mining support to bitcoind

  This PR adds the built-in stratum server and merge mining capability to our
  fork of Bitcoin Core.  This is the same stratum server that we use in
  Tradecraft, but adapted to bitcoin's different segwit implementation and with
  support for adding auxiliary commitments (e.g. to the Tradecraft chain).

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
