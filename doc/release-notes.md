Freicoin version 12.1.2-0 is now available from:

  * [Linux 32-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.2-0-linux32.zip)
  * [Linux 64-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.2-0-linux64.zip)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.2-0-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.2-0-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.2-0-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.2-0-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.2-0-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v12.1.2-0-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v12.1.2-0.zip)

This is a new point release, enabling soft-fork deployment of a
collection of time-lock related protocol features.

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

### Downgrade to a version < v10

Because release v10 and later makes use of headers-first
synchronization and parallel block download (see further), the block
files and databases are not backwards-compatible with pre-v10 versions
of Freicoin or other software:

* Blocks will be stored on disk out of order (in the order they are
  received, really), which makes it incompatible with some tools or
  other programs. Reindexing using earlier versions will also not work
  anymore as a result of this.

* The block index database will now hold headers for which no block is
  stored on disk, which earlier versions won't support.

If you want to be able to downgrade smoothly, make a backup of your
entire data directory. Without this your node will need start syncing
(or importing from bootstrap.dat) anew afterwards. It is possible that
the data from a completely synchronised 0.10 node may be usable in
older versions as-is, but this is not supported and may break as soon
as the older version attempts to reindex.

This does not affect wallet forward or backward compatibility.

### Downgrade to a version < v12

Because release v12 and later will obfuscate the chainstate on every
fresh sync or reindex, the chainstate is not backwards-compatible with
pre-v12 versions of Freicoin or other software.

If you want to downgrade after you have done a reindex with v12 or
later, you will need to reindex when you first start Freicoin version
v11 or earlier.

Notable changes
===============

First version bits BIP9 softfork deployment
-------------------------------------------

This release includes a soft fork deployment to enforce [BIP68][] and
[BIP113][] using the [BIP9][] deployment mechanism.

The deployment sets the block version number to 0x30000001 between
midnight 16nd April 2019 and midnight 2nd October 2019 to signal
readiness for deployment. The version number consists of 0x30000000 to
indicate version bits together with setting bit 0 to indicate support
for this combined deployment, shown as "locktime" in the
`getblockchaininfo` RPC call.

(The leading bits to indicate version bits is actually 0x20000000, but
version bits MUST be indicated and bit 28 set during this time period
due to the earlier deployment of the coinbase-MTP soft-fork.)

For more information about the soft forking change, please see
<https://github.com/bitcoin/bitcoin/pull/7648>

This specific backport pull-request to v0.12.1 of bitcoin, which this
release is based off of, can be viewed at
<https://github.com/bitcoin/bitcoin/pull/7543>

Unlike bitcoin, this soft-fork deployment does NOT include support for
[BIP112][], which provides the CHECKSEQUENCEVERIFY opcode. Support for
checking sequence locks in script will be added as part of the script
overhaul in segwit, scheduled for deployment with Freicoin v13.

For more information regarding these soft-forks, and the implications
for miners and users, please see the release notes accompanying
v12.1-10123.

[BIP9]: https://github.com/bitcoin/bips/blob/master/bip-0009.mediawiki
[BIP65]: https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki
[BIP68]: https://github.com/bitcoin/bips/blob/master/bip-0068.mediawiki
[BIP112]: https://github.com/bitcoin/bips/blob/master/bip-0112.mediawiki
[BIP113]: https://github.com/bitcoin/bips/blob/master/bip-0113.mediawiki

12.1.2-0 Change log
===================

Credits
--------

Thanks to who contributed to this release, including:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
