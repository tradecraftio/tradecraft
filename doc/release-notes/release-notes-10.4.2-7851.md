Freicoin version 10.4.2 is now available from:

  * [Linux 32-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.2-7851-linux32.zip)
  * [Linux 64-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.2-7851-linux64.zip)
  * [macOS (app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.2-7851-osx.dmg)
  * [macOS (server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.2-7851-osx64.tar.gz)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.2-7851-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.2-7851-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.2-7851-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v10.4.2-7851-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v10.4.2-7851.zip)

This is a new bug fix release, adding support for pool software that
rely on freicoind for the correct coinbase commitments, including
median time past. Upgrading to this release is required for miners
which use pool software that depends on the availability of the
"locktime" field from 'getblocktemplate,' but otherwise it is not
required.

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

Add "locktime" field to 'getblocktemplate' mining RPC
-----------------------------------------------------

The recent soft-fork to force vtx[0].nLockTime to be the current
median-time-past requires that mining software track freicoin headers
and generate this value from that information. In order to make this
process simpler for many downstream maintainers, we add the field
"locktime" to the results of the 'getblocktemplate' JSON-RPC call
which contains the required nLockTime value, which is equal to the
median of the past 11 block header timestamps.

10.4.2 changelog
================

- `a64790008` [Mining]
  Report the required coinbase nLockTime in 'getblocktemplate'.

Credits
--------

Thanks to who contributed to this release, including:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
