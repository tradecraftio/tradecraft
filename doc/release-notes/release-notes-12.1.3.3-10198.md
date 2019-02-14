Freicoin version 12.1.3.3-10198 is now available from:

  * [Linux 32-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.3-10198-linux32.zip)
  * [Linux 64-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.3-10198-linux64.zip)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.3-10198-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.3-10198-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.3-10198-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.3-10198-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.3-10198-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.3.3-10198-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v12.1.3.3-10198.zip)

This is a new patch release, adding a new stratum mining server and
fixing an erroneous warning about activation of version bit #28 during
the coinbase-mtp upgrade window.  Upgrading of mining nodes is
recommended, but not required.

Please report bugs using the issue tracker at github:

  https://github.com/tradecraftio/tradecraft/issues

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has
completely shut down (which might take a few minutes for older
versions), then run the installer (on Windows) or just copy over
/Applications/Freicoin-Qt (on Mac) or freicoind/freicoin-qt (on
Linux).

Downgrade warning
-----------------

### Downgrade to a version < v12

Because release v12 and later will obfuscate the chainstate on every
fresh sync or reindex, the chainstate is not backwards-compatible with
pre-v12 versions of Freicoin or other software.

If you want to downgrade after you have done a reindex with v12 or
later, you will need to reindex when you first start Freicoin version
v11 or earlier.

This does not affect wallet forward or backward compatibility.

Notable changes
===============

Ignore bit #28 during the coinbase-mtp activation window
--------------------------------------------------------

Due to the unique way in which the coinbase-mtp soft-fork was
activated, bit #28 remained set until the end of the activation
window, even once the new consensus rules were already in place.
Normal version bit behavior would have cleared the bit once activation
was achieved, so the version bits code interprets the still-set bit
#28 as the potential activation of a new, unknown rule.  This causes
the mining subsystem to halt, so as to not waste energy mining
potentially invalid blocks.

With this release, activation of bit #28 is ignored during the
coinbase-mtp activation window, so mining continues without stoppage
or warnings.

Add built-in stratum mining server with version-rolling support
---------------------------------------------------------------

This release includes, for the first time, a built-in stratum mining
server for solo-mining clients.  This service is established on port
9638 for main net, but can be configured with the `-stratumport`
option.  For security reasons the stratum server only accepts
connections from the loopback interface, but this default can be
overridden with the `-stratumallowip` and `-stratumbind` options.

It is anticipated that a future release will introduce a decentralized
mining pool allowing compensation for partial proof-of-work solutions.
However at this time the stratum server only provides solo-mining
work, with the entire payout of each block assigned to the miner that
solves it.

To use, configure your miner to connect to the interface and port
used, e.g. `stratum+tcp://127.0.0.1:9638`, and provide as the username
the freicoin address you desire to collect the mining proceeds.  The
username may optionally be suffixed with '+' followed by a numeric
minimum difficulty.  The password is unused and may be any value.

The stratum server supports the version-rolling extension for
ASCIBoost compatibility.

Protocol cleanup hard-fork
--------------------------

With this release, activation of the protocol cleanup hard-fork is
pushed to June 2022 at the earliest.

12.1.3.3-10198 Change log
=========================

  * `ce62d4d7` [Soft-fork]
    Do not warn for version bits activation of bit #28 during the
    coinbase-mtp activation window.

  * `371bc92d` [Consensus]
    Do not check that coinbase tx is final after activation of
    protocol cleanup hard-fork.

  * `8f2076e7` [Stratum]
    Implement core stratum mining protocol.

  * `b8a21679` [Stratum]
    Add version-rolling support to stratum server.

  * `e92334ce` [Hard-Fork]
    Push the protocol cleanup hard-fork activation to June 2022.

Credits
--------

Thanks to who contributed to this release, including:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
