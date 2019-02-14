v12.1.1-10130 Release Notes
===========================

Freicoin version v12.1.1-10130 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v12.1.1-10130

This is a new bug fix release, fixing an erroneous report of unexpected version / possible versionbits upgrade on all platforms.  Upgrading to this release is recommended but not required.

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

### Erroneous versionbits upgrade notice

The recent soft-fork to force vtx[0].nLockTime to be the current median-time-past requires that blocks signal for version bits and set bit 28, and to keep setting this bit once activated until 2 Oct 2019.  On [BIP9][] versionbits-aware clients such as v12.1, this set bit is erroneously interpreted as activation of an unknown consensus rule, and the user is alerted to upgrade.

This alert is generated in error and can be safely ignored in this case.  However this category of alert is serious and so as to prevent user complacency in the future we are issuing a new point release to eliminate this warning.

Beginning with v12.1.1, setting versionbit 28 will not trigger warnings about the activation of new consensus rules until 2 Oct 2019 (2 Apr 2019 on testnet).

[BIP9]: https://github.com/bitcoin/bips/blob/master/bip-0009.mediawiki

v12.1.1-10130 Change log
------------------------

- `658b0cde1` [Alert]
  Do not warn about unexpected versionbits upgrade for the forced setting of bit 28 before Oct 2, 2019.

- `f338bd9d4` [BIP9]
  The last deployed pre-versionbits block version on Freicoin was 3, not 4.

Credits
-------

Thanks to who contributed to this release, including:

- Fredrik Bodin
- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
