v23.0.0.1-32930 Release Notes
=============================

Bitcoin Core version v23.0.0.1-32930 is now available from:

  https://github.com/tradecraftio/mergemine/releases/tag/v23.0.0.1-32930

This is a new major release of the v23 stable branch of Bitcoin Core with the stratum mining server and Tradecraft/Freicoin merge-mining patches applied.

Please report bugs related to the stratum mining server implementation or Tradecraft/Freicoin merge-mining at the Tradecraft issue tracker on GitHub:

  https://github.com/tradecraftio/mergemine/issues

Please report other bugs regarding Bitcoin Core at the issue tracker on GitHub:

  https://github.com/bitcoin/bitcoin/issues

To receive security and update notifications for the stratum mining server and merge-mining patches, please subscribe to:

  https://tradecraft.groups.io/g/announce/

To receive security notifications for Bitcoin Core, please subscribe to:

  https://bitcoincore.org/en/list/announcements/join/

How to Upgrade
--------------

If you are running an older version, shut it down.  Wait until it has completely shut down (which might take a few minutes in some cases), then run the installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac) or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is possible, but it might take some time if the data directory needs to be migrated.  Old wallet versions of Bitcoin Core are generally supported.

Compatibility
-------------

Bitcoin Core is supported and extensively tested on operating systems using the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Bitcoin Core should also work on most other Unix-like systems but is not as frequently tested on them.  It is not recommended to use Bitcoin Core on unsupported systems.

From Bitcoin Core v23 onwards, macOS versions earlier than 10.15 are no longer supported.

Notable changes
---------------

There have been no notable changes in the stratum mining server or the Tradecraft/Freicoin merge-mining patches between v22.0.0.1-30022 and this release.  Please see the Bitcoin Core release notes for notable changes between the upstream versions upon which these releases are based.

Credits
-------

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
