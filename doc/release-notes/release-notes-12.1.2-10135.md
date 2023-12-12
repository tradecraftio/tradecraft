v12.1.2-10135 Release Notes
===========================

Freicoin version v12.1.2-10135 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v12.1.2-10135

This is a new point release, enabling soft-fork deployment of a collection of time-lock related protocol features.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes for older versions), then run the installer (on Windows) or just copy over /Applications/Freicoin-Qt (on Mac) or freicoind/freicoin-qt (on Linux).

Downgrade warning
-----------------

### Downgrade to a version < v12

Because release v12 and later will obfuscate the chainstate on every fresh sync or reindex, the chainstate is not backwards-compatible with pre-v12 versions of Freicoin or other software.

If you want to downgrade after you have done a reindex with v12 or later, you will need to reindex when you first start Freicoin version v11 or earlier.

This does not affect wallet forward or backward compatibility.

Notable changes
---------------

### First version bits BIP9 softfork deployment

This release includes a soft fork deployment to enforce [BIP68][] and [BIP113][] using the [BIP9][] deployment mechanism.

The deployment sets the block version number to 0x30000001 between midnight 16 April 2019 and midnight 2 October 2019 to signal readiness for deployment. The version number consists of 0x30000000 to indicate version bits together with setting bit 0 to indicate support for this combined deployment, shown as "locktime" in the `getblockchaininfo` RPC call.

(The leading bits to indicate version bits is actually 0x20000000, but version bits MUST be indicated and bit 28 set during this time period due to the earlier deployment of the coinbase-MTP soft-fork.)

For more information about the soft forking change, please see https://github.com/bitcoin/bitcoin/pull/7648

This specific backport pull-request to v0.12.1 of bitcoin, which this release is based off of, can be viewed at https://github.com/bitcoin/bitcoin/pull/7543

Unlike bitcoin, this soft-fork deployment does NOT include support for [BIP112][], which provides the CHECKSEQUENCEVERIFY opcode. Support for checking sequence locks in script will be added as part of the script overhaul in segwit, scheduled for deployment with Freicoin v13.

For more information regarding these soft-forks, and the implications for miners and users, please see the release notes accompanying v12.1-10123.

[BIP9]: https://github.com/bitcoin/bips/blob/master/bip-0009.mediawiki
[BIP65]: https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki
[BIP68]: https://github.com/bitcoin/bips/blob/master/bip-0068.mediawiki
[BIP112]: https://github.com/bitcoin/bips/blob/master/bip-0112.mediawiki
[BIP113]: https://github.com/bitcoin/bips/blob/master/bip-0113.mediawiki

v12.1.2-10135 Change log
------------------------

  * `26c3508b` [Chainparams]
    Add recent checkpoint, block #250992.

  * `989cc70c` [Consensus]
    Add activation logic for BIP68 and BIP113.

Credits
--------

Thanks to who contributed to this release, including:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
