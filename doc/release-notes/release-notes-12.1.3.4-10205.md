v12.1.3.4-10205 Release Notes
=============================

Freicoin version v12.1.3.4-10205 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v12.1.3.4-10205

This is a special purpose release of the pre-segwit, pre-auxpow v12 branch of the Tradecraft/Freicoin client for intended use within various infrastructure projects that require a v12 or earlier release.  Upgrading is necessary for those who need to continue using v12 releases, but upgrading to v13.2.4-11864 or higher is *highly* recommended wherever possible.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

WARNING
-------

The v12 branch does not distinguish between the protocol-cleanup rule changes and the relaxation of aggregate block limits in the size-expansion flag day.  Since v12 branch clients also do not support the auxiliary proof-of-work rules either, it is trivial for a malicious peer to perform a disk-space exhausting denial of service attack against v12 peers once the protocol-cleanup rule change activates on April 16th, 2021.

If you must run a v12 client, it is recommended that you firewall it behind another client which supports the auxiliary proof-of-work soft-fork, which was first released in v13.2.4-11864.  To help ensure that your v12 client does not connect to unknown peers, starting with this release will refuse to start unless it is configured to connect to a trusted peer with the `-connect` command line or configuration file option.  It is up to you to ensure that the node it connects to is properly updated.

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes for older versions), then run the installer (on Windows) or just copy over /Applications/Freicoin-Qt (on Mac) or freicoind/freicoin-qt (on Linux).

Notable changes
---------------

### Updated protocol-cleanup rule change flag day

The v13.2.4-11864 release took advantage of the deployment of auxiliary proof-of-work (AKA "merge-mining") to speed the activation of the protocol-cleanup rule changes, with the final date for activation chosen to be April 16th, 2021 in the v13.2.5-11909 release.  This support release of the v12 client includes this updated flag day activation parameters.

Known bugs
----------

### Split protocol-cleanup and size-expansion rule changes in future releases

Since a safe mechanism for handling block-size increases has yet to be written, those constraint relaxations have been split into a separate rule-change to be activated at some point in the future, no earlier than September 2nd, 2022.

However this split of the protocol-cleanup rule change into two separate activation flag days has NOT been back-ported to the v12 branch.  This release activates both the protocol-cleanup and size-expansion rule changes on April 16th, 2021.

Because of this, all v12 clients (including this and earlier releases) are vulnerable to disk-filling DoS attacks by untrusted peers.  If you must run a v12 client, it is essential to make sure the node only connects to trusted peers.  For this reason, starting with this release the client will refuse to start up if the `-connect` configuration option is not set.  Please use `-connect` to make sure your v12 node is connected to a v13.2.4-11864 or later peer that you control.

Credits
-------

Thanks to everyone who directly contributed to this release:

- Fredrik Bodin
- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
