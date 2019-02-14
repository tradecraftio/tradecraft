v13.2.5.2-11919 Release Notes
=============================

Freicoin version v13.2.5.2-11919 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v13.2.5.2-11919

This is a new patch release, containing a critical fix to the p2p block transmission code to fix a bug which prevents merge-mined blocks from being communicated to older clients.  Upgrading is highly recommended for nodes running v13.2.4-11864 or later releases.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

Notable changes
---------------

The recently released v13.2-11780 features activation parameters for the segregated witness (segwit) soft-fork, and dropped support for Microsoft Windows XP and macOS 10.7.  Please see the comprehensive notes accompanying v13.2-11780 for more information.

The recently released v13.2.4-11864 features activation parameters for the auxiliary proof-of-work (AKA "merge mining") soft-fork.  Please see the release notes accompanying v13.2.4-11864 for more information.

### Fix block propagation to non-segwit clients

The v13.2.4-11864 release introduced the auxiliary proof-of-work (AKA "merge mining") soft-fork, which has since activated on both testnet and mainnet.  Please see the release notes accompanying v13.2.4-11864 for more information.  Unbeknownst by anyone at the time, a bug in the serialization & p2p network code prevented auxiliary proof-of-work data structures from being stripped from block headers before being transmitted directly to non-segwit supporting clients (v12 or earlier).

Headers sent to more recent segwit-supporting nodes would be properly stripped and those nodes could relay the blocks to older clients, so this bug went unnoticed for some time.  Now that nodes supporting segwit but not auxiliary proof-of-work are becoming more infrequent, it was observed that v12 and earlier nodes are having trouble syncing from the network.

Although v12 and earlier nodes are not generally supported, this emergency patch release nevertheless fixes this serialization bug in order to re-enable syncing directly to these pre-segwit nodes.

Upgrading is highly recommended for all nodes currently running clients between v13.2.4-11864 and v14.3-13381.

Credits
-------

Thanks to everyone who directly contributed to this release:

- Fredrik Bodin
- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
