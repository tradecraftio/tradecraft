24.2-35816 Release Notes
========================

Freicoin version v24.2-35816 is now available from:

  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-aarch64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, big endian)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-powerpc64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, little endian)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-powerpc64le-linux-gnu.tar.gz)
  * [Linux RISC-V 64-bit (RV64GC)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-riscv64-linux-gnu.tar.gz)
  * [macOS (Apple Silicon, app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-arm64-apple-darwin.dmg)
  * [macOS (Apple Silicon, server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-arm64-apple-darwin.tar.gz)
  * [macOS (Intel, app)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-x86_64-apple-darwin.dmg)
  * [macOS (Intel, server)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-x86_64-apple-darwin.tar.gz)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/freicoin-v24.2-35816-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/v24.2-35816.zip)

This release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tradecraftio/tradecraft/issues>

To receive security and update notifications, please subscribe to:

  <https://tradecraft.groups.io/g/announce/>

How to Upgrade
==============

If you are running an older version, shut it down.  Wait until it has
completely shut down (which might take a few minutes in some cases), then run
the installer (on Windows) or just copy over `/Applications/Freicoin-Qt` (on
Mac) or `freicoind`/`freicoin-qt` (on Linux).

Upgrading directly from a version of Freicoin that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated.  Old wallet versions of Freicoin are generally supported.

Compatibility
==============

Freicoin is supported and extensively tested on operating systems using the
Linux kernel, macOS 10.15+, and Windows 7 and newer.  Freicoin should also
work on most other Unix-like systems but is not as frequently tested on them.
It is not recommended to use Freicoin on unsupported systems.

Notable changes
===============

Fees
----

- #27622 Fee estimation: avoid serving stale fee estimate

RPC and other APIs
------------------

- #27727 rpc: Fix invalid bech32 address handling

Build System
------------

- #28097 depends: xcb-proto 1.15.2
- #28543 build, macos: Fix qt package build with new Xcode 15 linker
- #28571 depends: fix unusable memory_resource in macos qt build

CI
--

- #27777 ci: Prune dangling images on RESTART_CI_DOCKER_BEFORE_RUN
- #27834 ci: Nuke Android APK task, Use credits for tsan
- #27844 ci: Use podman stop over podman kill
- #27886 ci: Switch to amd64 container in "ARM" task

Miscellaneous
-------------

- #28452 Do not use std::vector = {} to release memory

Credits
=======

Thanks to everyone who directly contributed to this release:

- Abubakar Sadiq Ismail
- Hennadii Stepanov
- Marco Falke
- Mark Friedenbach
- Michael Ford
- Pieter Wuille

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
