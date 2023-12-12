v13.2.1-11797 Release Notes
===========================

Freicoin version v13.2.1-11797 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v13.2.1-11797

This is a new patch release, featuring a critical fix related to database corruption on startup, and a small number of other improvements.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

Notable changes
---------------

The recently released v13.2-11780 features activation parameters for the segregated witness (segwit) soft-fork, and dropped support for Microsoft Windows XP and macOS 10.7.  Please see the comprehensive notes accompanying v13.2-11780 for more information.

### Fix corruption of block-final tx hash on startup

Previous releases sometimes hang on start and require restarting with `-reindex=1`, and clearing the ban list in order to make progress again.  It turns out the root cause of this error is that some invalid cache data is being flushed to disk at one or more points during startup.  The flushing of invalid state is fixed in this release, and logic is added to cause the server to reindex from activation of the block-final tx rules if corruption is detected (as might be be the case with a newly upgraded client).

### Stratum mining server

Added support for the `mining.extranonce.subscribe` method, which is required by some mining clients.  This only affects behavior for clients which send the server a `mining.extranonce.subscribe` RPC, which causes the server to send a `mining.set_extranonce` message on each new work notification.

Additionally, the stratum server is now disabled by default in order to prevent it being a potential attack vector, and must be enabled with `-stratum=1`.

### Warning: unknown version (removed)

Previous releases generated a warning to the user and connected mining clients that unknown versions were being observed on the network.  This was due to widespread support of version-rolling mining extensions (also known as "overt ASICBoost"), which causes some bits to be flipped during normal operation.  In a future release we will introduce more stringent upgrade detection logic, but for this release the warning has simply been removed.  The number of unknown versions seen in the last 100 blocks is still recorded in the log.

v13.2.1-11797 Change log
------------------------

The following pull requests were merged into the Tradecraft code repository for this release.

- tradecraftio/tradecraft#71 [Stratum] Added support for the `mining.extranonce.subscribe` method.

  From the logs on the public pool servers, it appears that some miner's clients rely on the `mining.extranonce.subscribe` stratum feature, even though its purpose is entirely redundant AFAICT.  But whatever, it's simple enough to support.

  This commit makes the server accept a `mining.extranonce.subscribe` command, which makes it send an extra message to that miner informing it, yet again, of the extranonce settings each time work is generated.  The extranonce settings DO NOT change from what was returned to the miner when they first connected and sent a `mining.subscribe` message.

- tradecraftio/tradecraft#73 [Stratum] Disable server by default, and fix RPC tests.

  For security reasons, it is preferable to have the stratum server disabled by default.  It's just one less potential attack interface.  This also fixes the rpcbind RPC test.  While we're at it, this PR also fixes the stratum port assignment for RPC tests, so that multiple nodes don't conflict over the same port.  Not that this matters, since there aren't any stratum RPC tests yet and it is disabled by default, but it'll prevent a potential bug once we do write some stratum tests.

- tradecraftio/tradecraft#72 [Warn] Do not warn about unknown version bits set.

  Having unspecified version bit flags set is normal now that the industry has transitioned to version-rolling mining hardware.  The upstream 0.13 release of bitcoin raises a warning if more than 50 of the last 100 blocks show unrecognized version numbers.  In the current network, this is almost always the case.  This PR disables the warning, and disables the RPC tests which check for the warning.

- tradecraftio/tradecraft#74 [FinalTx] Fix block-final tx related database corruption on v13.

  It was been observed in PR #70 that v13 sometimes hangs on start and requires restarting with `-reindex=1`, and clearing the ban list in order to make progress again.  It turns out the root cause of this error is that some invalid cache data is being flushed to disk at one or more points during startup.  This PR includes a number of changes related to fixing this problem:

    1. The debug output is improved with changes that were helpful in isolating the error.

    2. A clean node shutdown is triggered if corruption is detected during operation, rather than entering a peer-banning infinite loop as is the observed behavior on v13.2-11780.

    3. During initialization the block-final tx hash in the database is checked, and if not valid (whether missing or corrupted), a reindex from the point of activation of the block-final fork is triggered.  This is modeled on the behavior of an upgraded segwit node, which also resynchronizes from the point of activation on first startup after upgrading.

    4. Explicitly track which fields in the cache are valid, so as to not write invalid data to the database when flushed.

  (4) actually fixes the root cause; (1) - (3) are to make sure that if a similar issue arises again, the code will be smart enough to restore its internal state and recover operation, and to recover state for any corrupted nodes in the wild that upgrade in the next release.

  Also, in testing it was observed that this fixes #48.  It turns out the lurking reorg-invalidation problem has this flushing of invalid cache data as its root cause too!  We therefore re-enable the pruning RPC test which was previously clobbered by the block-final tx changes.

Credits
-------

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
