v13.2.5.1-11914 Release Notes
=============================

Freicoin version v13.2.5.1-11914 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v13.2.5.1-11914

This is a new patch release, containing a critical fix to the mining block generation code to fix a bug which prevents new work from being generated.  Upgrading is required for mining nodes currently running v13.2.5-11909.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

Notable changes
---------------

The recently released v13.2-11780 features activation parameters for the segregated witness (segwit) soft-fork, and dropped support for Microsoft Windows XP and macOS 10.7.  Please see the comprehensive notes accompanying v13.2-11780 for more information.

The recently released v13.2.4-11864 features activation parameters for the auxiliary proof-of-work (AKA "merge mining") soft-fork.  Please see the release notes accompanying v13.2.4-11864 for more information.

### Fixes work generation for mining nodes

A bug in the block validity testing code in v13.2.5-11909 prevents new work from being generated and served to miners.  Due to a refactoring, block templates without valid proof-of-work are rejected, so tests of block template validity cause the mining code to fail, causing v13.2.5-11909 to be unable to generate work for miners.

This v13.2.5.1-11914 emergency patch release fixes this bug, restoring the expected behavior for the mining block template validation code, as well any other follow-on bugs that likely exist but remain unidentified.

Upgrading (or downgrading to a pre-v13.2.5 release) is highly recommended for all mining nodes currently running v13.2.5-11909.

Credits
-------

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
