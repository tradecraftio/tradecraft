v24.0.1.2-35544 Release Notes
=============================

Bitcoin Core version v24.0.1.2-35544 is now available from:

  * [Linux x86_64 (Intel 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-x86_64-linux-gnu.tar.gz)
  * [Linux ARMv7-A (ARM 32-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-arm-linux-gnueabihf.tar.gz)
  * [Linux ARMv8-A (ARM 64-bit)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-aarch64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, big endian)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-powerpc64-linux-gnu.tar.gz)
  * [Linux PowerPC (64-bit, little endian)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-powerpc64le-linux-gnu.tar.gz)
  * [Linux RISC-V 64-bit (RV64GC)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-riscv64-linux-gnu.tar.gz)
  * [macOS (Apple Silicon, app)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-arm64-apple-darwin.dmg)
  * [macOS (Apple Silicon, server)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-arm64-apple-darwin.tar.gz)
  * [macOS (Intel, app)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-x86_64-apple-darwin.dmg)
  * [macOS (Intel, server)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-x86_64-apple-darwin.tar.gz)
  * [Windows 64-bit (installer)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-win64-setup.exe)
  * [Windows 64-bit (zip)](https://s3.amazonaws.com/in.freico.stable/bitcoin-v24.0.1.2-35544-win64.zip)
  * [Source](https://github.com/tradecraftio/tradecraft/archive/bitcoin-v24.0.1.2-35544.zip)

This is a new patch release of the v24 stable branch of Bitcoin Core with the
stratum mining server and Tradecraft/Freicoin merge-mining patches applied.
This release adds new capabilities to the stratum mining server.

Please report bugs related to the stratum mining server implementation or
Tradecraft/Freicoin merge-mining at the Tradecraft issue tracker on GitHub:

  <https://github.com/tradecraftio/tradecraft/issues>

Please report other bugs regarding Bitcoin Core at the issue tracker on GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications for the stratum mining server and
merge-mining patches, please subscribe to:

  <https://tradecraft.groups.io/g/announce/>

To receive security notifications for Bitcoin Core, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has
completely shut down (which might take a few minutes in some cases), then run
the installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on
macOS) or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated.  Old wallet versions of Bitcoin Core are generally supported.

Compatibility
=============

Bitcoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Bitcoin Core should
also work on most other Unix-like systems but is not as frequently tested on
them. It is not recommended to use Bitcoin Core on unsupported systems.

Notable changes
===============

As a patch release, v24.0.1.2-35544 contains new features, bug fixes, and
performance improvements specific to the stratum mining server and
Tradecraft/Freicoin merge-mining patches.  Specifically, v24.0.1.2-35544 adds
two major improvements to the stratum mining server:

Merge-mining multiple chains
----------------------------

The stratum mining server now supports merge-mining multiple chains at once.
This is a capability which was always supported by the merge-mining protocol,
but required some code changes to enable.  The stratum mining server now
supports merge-mining not just Bitcoin and Tradecraft/Freicoin at the same
time, but also the Tradecraft/Freicoin testnet, any other chains that support
the merge-mining protocol.

To merge-mine multiple chains at once, simply include the authorization
information for all chains in a comma-separated list as the stratum
authentication password. For example:

    $ cgminer -o stratum+tcp://[host]:[port]
              -u bc1qabc
              -p freicoin:fc1qdef,testnet:tc1qxyz

Default authorization credentials
---------------------------------

Users of the stratum mining server provide a bitcoin address to receive the
bitcoin block reward, and a Tradecraft/Freicoin address to authenticate with
the merge-mined chain's stratum server for its block reward.  Previously the
bitcoin address as username was required, and the Tradecraft/Freicoin address
was necessary for the user to merge-mine Tradecraft/Freicoin.  With this
release, the stratum mining server now supports authorization defaults.

If the stratum miner does not provide a username (or the username field is
empty), then the configuration option `-defaultminingaddress` is used as the
bitcoin address for that user.  If this configuration option is not set, then
the option `-stratumwallet` specifies the name of a wallet from which to
generate an address (or if given a non-zero value that does not correspond to
a wallet, then the default/first wallet is used).  When used in this mode, a
new address is generated for each found block.

Merge-mining defaults can similarly be specified with the `-mergeminedefault`
option, which takes a name/chain ID, a username/address, and an optional
password to be used to authenticate with the merge-mined chain's stratum
server.  This configuration option may be repeated to provide defaults for
different chains.

Finally, the merge-mining auxiliary work server itself can specify a default
username and password to use for all clients which have not specified their
own credentials for that chain.  In that case, these credentials are used for
miners which do not provide their own credentials (unless `-mergeminedefault`
is also specified for that chain, which overrides the auxiliary server).

Credits
=======

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
