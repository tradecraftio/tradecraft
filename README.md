Freicoin integration/staging tree
=====================================

http://freico.in

For an immediately usable, binary version of the Freicoin software, see
http://freico.in/en/download/.

What is Freicoin?
---------------------

Freicoin connects to the Freicoin peer-to-peer network to download and fully
validate blocks and transactions. It also includes a wallet and graphical user
interface, which can be optionally built.

Further information about Freicoin is available in the [doc folder](/doc).

License
-------

Freicoin is released under the terms of version 3 of the GNU Affero
General Public License as published by the Free Software Foundation. See
[COPYING](COPYING) for more information.

Development Process
-------------------

Development takes place on numbered branches corresponding to upstream releases
of Bitcoin Core.  The `21` branch is based on the upstream `bitcoin/0.21`
branch and is is regularly built and tested, but is not guaranteed to be
completely stable.  [Tags](https://github.com/tradecraftio/tradecraft/tags) are created
regularly to indicate new official, stable release versions of Freicoin.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md)
and useful hints for developers can be found in [doc/developer-notes.md](doc/developer-notes.md).

The developer [mailing list](https://tradecraft.groups.io/g/devel/)
should be used to discuss complicated or controversial changes before working
on a patch set.

Developer IRC can be found on Libera at #freicoin.

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python.
These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

The CI (Continuous Integration) systems make sure that every pull request is built for Windows, Linux, and macOS,
and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Freicoin's Transifex page](https://www.transifex.com/tradecraft/freicoin-1/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.
