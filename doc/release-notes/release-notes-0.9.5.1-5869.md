Freicoin version 0.9.5.1 is now available from:

  * [Linux 32-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v0.9.5.1-5869-linux32.zip)
  * [Linux 64-bit](https://s3.amazonaws.com/in.freico.stable/freicoin-v0.9.5.1-5869-linux64.zip)
  * [macOS](https://s3.amazonaws.com/in.freico.stable/freicoin-v0.9.5.1-5869-osx.dmg)
  * [Windows 32-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v0.9.5.1-5869-win32-setup.exe)
  * [Windows 32-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v0.9.5.1-5869-win32.zip)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v0.9.5.1-5869-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v0.9.5.1-5869-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v0.9.5.1-5869.zip)

This is a new minor version release, with the goal of backporting
soft-fork activation rules for verification of the coinbase
transaction's nLockTime field. There is also a modification of the
wallet code to produce low-S signatures, and various bug
fixes. Upgrading to this release is recommended.

Please report bugs using the issue tracker at github:

  https://github.com/tradecraftio/tradecraft/issues

How to Upgrade
===============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Freicoin-Qt (on Mac) or
freicoind/freicoin-qt (on Linux).

Notable changes
================

Soft-fork activation rules for coinbase nLockTime
-------------------------------------------------

This rule update requires the nTimeLock value of a coinbase
transaction to be equal to the median of the past 11 block times. It
uses an old-style supermajority rollout with BIP8/BIP9-like version
bits and BIP8-like activation-on-timeout semantics. It's not strictly
compatible with those BIPs though, as we didn't want to back port all
the necessary state management code. But miners using BIP8/BIP9
compatible version bit software should work with this rollout by
configuring their devices to signal bit #28 (the highest-order
BIP8/BIP9 bit).

Wallet generates signatures with low-S value
--------------------------------------------

The prior release included a modification to the wallet signing code
to produce signatures with quadratic residue R values rather than low
(< half the order) S values. Given the structure of the signing code,
we can actually do both, and this release always generates quadratic
R, low S signatures.

Remove version-upgrade warnings
-------------------------------

The version upgrade warning implemented in the 0.9 series is unable to
properly handle version bits signaling, and falsely identifies BIP8 /
BIP9 blocks as being produced by a future version, thereby triggering
upgrade warnings on up-to-date clients. This upgrade warning has been
removed; future releases will include proper handling for version-bits
blocks.

Reverse demurrage code
----------------------

Example code, currently unused, is provided for doing "reverse"
demurrage calculations, taking a present value amount and calculating
what it would have been at an earlier epoch. This fast code uses only
fixed-point integer math.

Build system upgrades
---------------------

Due to deprecation of some build dependencies, the build system has
been modified to bootstrap itself from alternative sources.

0.9.5.1 changelog
=================

- `59cb913b` [Soft-fork]
  Require the nTimeLock value of a coinbase transaction to be equal to
  the median of the past 11 block times.

- `e62b1f35` [OpenSSL]
  Generate signatures with low-S values.

- `e37ec652` [Alert]
  Do not warn that an upgrade is required when unrecognized block
  versions have been observed.

- `9f12779b` [Demurrage]
  Add inverse-demurrage calculations, permitting GetTimeAdjustedValue
  to be called with negative relative_depth values and yield
  reasonable results.

- `04006641` [Vagrant]
  Switch to our own hosted macOS 10.7 SDK dependency, required for
  pre-0.11 releases.

- `9f399309` [Vagrant]
  Update download URL for libpng-1.6.8.tar.gz, since the original
  server is no longer online.

- `eec192d4` [Vagrant]
  Update from Ubuntu 16.04.4 to 16.04.5, since the .4 images are no
  longer hosted by Canonical.

Credits
--------

Thanks to who contributed to this release, including:

- Fredrik Bodin
- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
