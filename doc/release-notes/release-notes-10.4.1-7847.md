Freicoin version 10.4.1 is now available from:

  * [Linux 32-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.1-7847-linux32.zip)
  * [Linux 64-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.1-7847-linux64.zip)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.1-7847-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.1-7847-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.1-7847-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.1-7847-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.1-7847-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.1-7847-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v10.4.1-7847.zip)

This is a new bug fix release, fixing an issue with incorrect display
of immature and watch-only balances on all platforms. Upgrading to
this release is recommended for those affected by this issue, but
otherwise not required.

Please report bugs using the issue tracker at github:

  https://github.com/tradecraftio/tradecraft/issues

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has
completely shut down (which might take a few minutes for older
versions), then run the installer (on Windows) or just copy over
/Applications/Freicoin-Qt (on Mac) or freicoind/freicoin-qt (on
Linux).

Notable changes
===============

Fix of incorrect display of immature and watch-only balances
------------------------------------------------------------

Version 10.4 introduced the new feature of watch-only addresses,
which are scripts the wallet tracks as if it had the private keys,
a construct which finds use in many circumstances such as online
tracking of a cold-storage wallet. Unfortunately the wallet routines
for calculating watch-only balances were not updated to apply
demurrage to these amounts. As a result, the display for a watch-only
wallet shows an incorrectly inflated amount of total freicoin compared
to what is actually available.

While fixing this bug it was re-discovered that immature balances were
also being displayed improperly, the result of a conscious decision in
a prior release for reasons that no longer apply.

Both issues have been fixed, and the GUI and RPC commands which
calculate wallet balances now correctly show demurrage-adjusted
totals.

10.4.1 changelog
================

- `52742c45b` [Wallet]
  Correctly apply demurrage to watch-only and immature balance
  calculations.

Credits
--------

Thanks to who contributed to this release, including:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
