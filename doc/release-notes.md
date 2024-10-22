v27.1.1-40580 Release Notes
===========================

Freicoin version v27.1.1-40580 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v27.1.1-40580

This release includes critical bug fixes related to activation of the protocol cleanup rule changes.  Upgrading is highly recommended.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

How to Upgrade
--------------

If you are running an older version, shut it down.  Wait until it has completely shut down (which might take a few minutes in some cases), then run the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on macOS) or `freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is possible, but it might take some time if the data directory needs to be migrated.  Old wallet versions of Freicoin are generally supported.

If upgrading from a release prior to v27.1.1-40580, it is highly recommended for the health of the network and your node's durability that you remove the "blocks" and "chainstate" subdirectories of your data directory before resuming, therefore forcing a redownload of the block chain.  This will ensure that you have the capability to rebuild your local block database by resuming with `-reindex=1` in the future, if needed.

Compatibility
-------------

Freicoin is supported and extensively tested on operating systems using the Linux Kernel 3.17+, macOS 11.0+, and Windows 7 and newer.  Freicoin should also work on most other Unix-like systems but is not as frequently tested on them.  It is not recommended to use Freicoin on unsupported systems.

Notable changes
---------------

## P2P

Blocks received from the network are updated to include the auxiliary proof-of-work information, if an auxpow is found in the block index but missing in the received block.  This fixes a pervasive bug in previous editions whereby a block received from an old client and not containing an auxpow would nevertheless be accepted (as the auxpow-containing header had already been seen and validated), and the block stored on disk without its auxiliary proof-of-work.  Other peers performing IBD from this node would then receive the auxpow-missing block, perpetuating the problem.  No trouble would be encountered until the new node needed to reindex, in which case it would stop at the first auxpow-missing block and switch to IBD.

Credits
-------

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
