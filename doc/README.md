Freicoin
=============

Setup
---------------------
Freicoin is the original Freicoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Freicoin transactions, which requires a few hundred gigabytes of disk space. Depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

To download Freicoin, visit [freico.in](http://freico.in/en/download/).

Running
---------------------
The following are some helpful notes on how to run Freicoin on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/freicoin-qt` (GUI) or
- `bin/freicoind` (headless)

### Windows

Unpack the files into a directory, and then run freicoin-qt.exe.

### macOS

Drag Freicoin to your applications folder, and then run Freicoin.

### Need Help?

* See the documentation at the [Bitcoin Wiki](https://en.bitcoin.it/wiki/Main_Page)
for help and more information.
* Ask for help on [#freicoin](http://web.libera.chat?channels=freicoin) on Libera. If you don't have an IRC client, use [webchat here](http://web.libera.chat?channels=freicoin).
* Ask for help on the [BitcoinTalk](https://bitcointalk.org/) forums, in the [Technical Support board](https://bitcointalk.org/index.php?board=4.0).

Building
---------------------
The following are developer notes on how to build Freicoin on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [FreeBSD Build Notes](build-freebsd.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Gitian Building Guide (External Link)](https://github.com/freicoin/docs/blob/master/gitian-building.md)

Development
---------------------
The Freicoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Productivity Notes](productivity.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/freicoin/doxygen/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [JSON-RPC Interface](JSON-RPC-interface.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [BitcoinTalk](https://bitcointalk.org/) forums, in the [Development & Technical Discussion board](https://bitcointalk.org/index.php?board=6.0).
* Discuss project-specific development on #freicoin on Libera. If you don't have an IRC client, use [webchat here](http://web.libera.chat/?channels=freicoin).
* Discuss general Freicoin development on #freicoin-dev on Libera. If you don't have an IRC client, use [webchat here](http://web.libera.chat/?channels=freicoin-dev).

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [freicoin.conf Configuration File](freicoin-conf.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)
- [PST support](pst.md)

License
---------------------
Distributed under the [GNU Affero General Purpose License v3.0](https://www.gnu.org/licenses/agpl-3.0.en.html).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
