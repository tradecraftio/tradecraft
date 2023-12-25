v26.0.2-39498 Release Notes
===========================

Freicoin version v26.0.2-39498 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v26.0.2-39498

This release includes various bug fixes and compatibility improvements related to the stratum mining subsystem.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes in some cases), then run the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on macOS) or `freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is possible, but it might take some time if the data directory needs to be migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
-------------

Freicoin is supported and extensively tested on operating systems using the Linux kernel, macOS 11.0+, and Windows 7 and newer.  Freicoin should also work on most other Unix-like systems but is not as frequently tested on them.  It is not recommended to use Freicoin on unsupported systems.

Notable changes
---------------

There are no notable, user-facing changes between this release and the prior release in this series, v26.0.1-39423.  However, in the process of fixing build errors when compiling from within Microsoft Visual Studio, which is now supported, a number of logic errors and undefined behavior conditions were uncovered and fixed.

The undefined behavior related to popping from an empty stack, which is then deallocated.  With the standard template library used in the release builds, this is not a problem, but it does produce a memory access violation when compiled with Microsoft Visual Studio.  This instance of undefined behavior is now fixed, enabling Freicoin to be compiled and run natively from within Microsoft Visual Studio, without the cross compilation infrastructure used to generate release builds.

Additionally, the proper locks are now taken in the stratum mining subsystem before accessing shared resources, such as the current chain state.  In the unlikely event of a data race this could have led to stability issues, although the shared state is updated so infrequently that it is not surprising that these bugs have never been encountered in the wild.  Nevertheless, upgrading to this release is recommended for all users.

Credits
-------

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
