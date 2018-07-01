#!/bin/bash
# Copyright (c) 2019-2023 The Freicoin Developers.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of version 3 of the GNU Affero General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public
# License along with this program.  If not, see
# <https://www.gnu.org/licenses/>.

set -e

if [ -e share/examples/bitcoin.conf ]; then
  for fn in `git ls-tree -r --name-only HEAD | grep bitcoin`; do
    oldfn=`basename "$fn"`
    newfn=`basename "$fn" | sed -e 's:bitcoin:freicoin:g'`
    if [ "$oldfn" != "$newfn" ]; then
      git mv "$fn" `dirname "$fn"`/"$newfn"
    fi
  done
  for fn in `git ls-tree -r --name-only HEAD | grep psbt`; do
    oldfn=`basename "$fn"`
    newfn=`basename "$fn" | sed -e 's:psbt:pst:g'`
    if [ "$oldfn" != "$newfn" ]; then
      git mv "$fn" `dirname "$fn"`/"$newfn"
    fi
  done
  git commit -m '[Branding] Rename files "bitcoin" -> "freicoin".'
fi

git grep -z -l bitcoin | xargs -0 sed -i 's:bitcoin:freicoin:g'
git grep -z -l Bitcoin | xargs -0 sed -i 's:Bitcoin:Freicoin:g'
git grep -z -l BitCoin | xargs -0 sed -i 's:BitCoin:FreiCoin:g'
git grep -z -l BITCOIN | xargs -0 sed -i 's:BITCOIN:FREICOIN:g'
git grep -z -l BTC | xargs -0 sed -i 's:BTC:FRC:g'
git grep -z -l btc | xargs -0 sed -i 's:btc:frc:g'
git grep -z -l satoshis-per | xargs -0 sed -i 's:satoshis-per:kria-per:g'
git grep -z -l "satoshi(s)" | xargs -0 sed -i 's:satoshi(s):kria:g'
git grep -z -l satoshis | xargs -0 sed -i 's:satoshis:kria:g'
git grep -z -l satoshi | xargs -0 sed -i 's:satoshi:kria:g'
git grep -z -l SatoshisPer | xargs -0 sed -i 's:SatoshisPer:KriaPer:g'
git grep -z -l Satoshis | xargs -0 sed -i 's:Satoshis:kria:g'

# Remove "bitcoin" from "partially signed bitcoin transaction"
git grep -z -l psbt | xargs -0 sed -i 's:psbt:pst:g'
git grep -z -l PSBT | xargs -0 sed -i 's:PSBT:PST:g'
git grep -z -l 'b"pst' | xargs -0 sed -i 's:b"pst:b"psbt:g'

# We want old-style signed messages to remain compatible with Bitcoin
git grep -z -l "Freicoin Signed Message" | xargs -0 sed -i 's:Freicoin Signed Message:Bitcoin Signed Message:g'

# Update an index into a hard-coded "bitcoin:" string
git grep -z -l "replace(0, 10," | xargs -0 sed -i 's:replace(0, 10,:replace(0, 11,:g'

# We don't want to rebrand the copyright declaration
git grep -z -l Copyright | xargs -0 sed -z -i 's:Copyright\(.*\)Freicoin\(.*\)Copyright\(.*\)Freicoin:Copyright\1Bitcoin\2Copyright\3Freicoin:g'

# There's a couple of vanity bitcoin onion addresses.  They get removed later
# since they don't apply to the Freicoin network, but let's not have invalid
# addresses in the codebase.
git grep -z -l freicoin7bi4op7wb.onion | xargs -0 sed -i 's:freicoin7bi4op7wb.onion:bitcoin7bi4op7wb.onion:g'
git grep -z -l freicoinostk4e4re.onion | xargs -0 sed -i 's:freicoinostk4e4re.onion:bitcoinostk4e4re.onion:g'
git grep -z -l thfsmmn2jfreicoin.onion | xargs -0 sed -i 's:thfsmmn2jfreicoin.onion:thfsmmn2jbitcoin.onion:g'

# The IRC channel is on Freenode and doesn't have the '-dev' suffix
git grep -z -l 'webchat.freenode.net' | xargs -0 sed -i 's:webchat.freenode.net:web.libera.chat:g'
git grep -z -l 'Freenode' | xargs -0 sed -i 's:Freenode:Libera:g'
git grep -z -l 'freenode' | xargs -0 sed -i 's:freenode:libera:g'
git grep -z -l '#freicoin-core-dev' | xargs -0 sed -i 's:#freicoin-core-dev:#freicoin:g'
git grep -z -l 'channels=freicoin-core-dev' | xargs -0 sed -i 's:channels=freicoin-core-dev:channels=freicoin:g'

git grep -z -l Freicoin\ Core | xargs -0 sed -i 's:Freicoin Core:Freicoin:g'
git grep -z -l Freicoin\ core | xargs -0 sed -i 's:Freicoin core:Freicoin:g'
git grep -z -l freicoin\ core | xargs -0 sed -i 's:freicoin core:freicoin:g'
git grep -z -l Freicoin-Core | xargs -0 sed -i 's:Freicoin-Core:Freicoin:g'
git grep -z -l freicoin-core | xargs -0 sed -i 's:freicoin-core:freicoin:g'
git grep -z -l "2009-%1 \").arg(COPYRIGHT_YEAR) + QString(tr(\"The Freicoin" | xargs -0 sed -i 's:2009-%1 ").arg(COPYRIGHT_YEAR) + QString(tr("The Freicoin:2009-%1 ").arg(COPYRIGHT_YEAR) + QString(tr("The Bitcoin Core:g'
git grep -z -l Freicoin\ Wiki | xargs -0 sed -i 's:Freicoin Wiki:Bitcoin Wiki:g'
git grep -z -l freicointalk | xargs -0 sed -i 's:freicointalk:bitcointalk:g'
git grep -z -l FreicoinTalk | xargs -0 sed -i 's:FreicoinTalk:BitcoinTalk:g'
git grep -z -l freicoin.org | xargs -0 sed -i 's:freicoin.org:freico.in:g'
git grep -z -l www.freico.in | xargs -0 sed -i 's:www.freico.in:freico.in:g'
git grep -z -l freicoin/freico.in | xargs -0 sed -i 's:freicoin/freico.in:bitcoin/bitcoin.org:g'
git grep -z -l freicoin.sipa.be | xargs -0 sed -i 's:freicoin.sipa.be:bitcoin.sipa.be:g'
git grep -z -l org.freicoinfoundation | xargs -0 sed -i 's:org.freicoinfoundation:in.freico:g'
git grep -z -l '=org/freicoin' | xargs -0 sed -i 's:=org/freicoin:=in/freico:g'
git grep -z -l org_freicoin | xargs -0 sed -i 's:org_freicoin:in_freico:g'
git grep -z -l 'org\.freicoin' | xargs -0 sed -i 's:org\.freicoin:in.freico:g'
git grep -z -l freicoinfoundation | xargs -0 sed -i 's:freicoinfoundation:bitcoinfoundation:g'
git grep -z -l freicoincore.org | xargs -0 sed -i 's:freicoincore.org:freico.in:g'
git grep -z -l 'freicoin\.it' | xargs -0 sed -i 's:freicoin\.it:bitcoin.it:g'
git grep -z -l 'transifex\.com/freicoin/freicoin' | xargs -0 sed -i 's:transifex\.com/freicoin/freicoin:transifex.com/tradecraft/freicoin-1:g'

# Don't change the guix build dependencies
git grep -z -l 'freicoin-linux-g++' | xargs -0 sed -i 's:freicoin-linux-g++:bitcoin-linux-g++:g'

# We have our own hosting for hard-to-find build dependencies like the macOS SDK
git grep -z -l 'freico\.in/depends-sources/sdks' | xargs -0 sed -i 's:freico.in/depends-sources/sdks:raw.githubusercontent.com/maaku/dependencies/master:g'

# Current website doesn't support TLS
git grep -z -l 'https://freico\.in' | xargs -0 sed -i 's;https://freico\.in;http://freico.in;g'

# Keep references to Bitcoin Core issues...
git grep -z -l 'freicoin/freicoin/issues/' | xargs -0 sed -i 's:freicoin/freicoin/issues/:bitcoin/bitcoin/issues/:g'
git grep -z -l 'freicoin/freicoin/pull/' | xargs -0 sed -i 's:freicoin/freicoin/pull/:bitcoin/bitcoin/pull/:g'

# ...but change all other reference to the repo
git grep -z -l 'github\.com/freicoin/freicoin' | xargs -0 sed -i 's:github\.com/freicoin/freicoin:github.com/tradecraftio/tradecraft:g'
git grep -z -l 'github/freicoin/freicoin' | xargs -0 sed -i 's:github/freicoin/freicoin:github/tradecraftio/tradecraft:g'
git grep -z -l 'freicoin/freicoin repo' | xargs -0 sed -i 's:freicoin/freicoin repo:tradecraftio/tradecraft repo:g'
git grep -z -l 'repository freicoin/freicoin' | xargs -0 sed -i 's:repository freicoin/freicoin:repository tradecraftio/tradecraft:g'
git grep -z -l 'sipa/freicoin-seeder' | xargs -0 sed -i 's:sipa/freicoin-seeder:freicoin/bitcoin-seeder:g'

# Restore references to other repos that we are not hosting
git grep -z -l 'freicoin/secp256k1' | xargs -0 sed -i 's:freicoin/secp256k1:bitcoin-core/secp256k1:g'
git grep -z -l 'freicoin/leveldb' | xargs -0 sed -i 's:freicoin/leveldb:bitcoin-core/leveldb:g'
git grep -z -l 'branch freicoin-fork' | xargs -0 sed -i 's:branch freicoin-fork:branch bitcoin-fork:g'
git grep -z -l 'freicoin/univalue' | xargs -0 sed -i 's:freicoin/univalue:bitcoin-core/univalue:g'
git grep -z -l 'freicoin/ctaes' | xargs -0 sed -i 's:freicoin/ctaes:bitcoin-core/ctaes:g'
git grep -z -l 'freicoin/bips' | xargs -0 sed -i 's:freicoin/bips:bitcoin/bips:g'

# Restore gitian instructions
git grep -z -l 'gitian-freicoin-host' | xargs -0 sed -i 's:gitian-freicoin-host:gitian-bitcoin-host:g'

# And a few filesystem references
git grep -z -l 'tradecraftio/tradecraftd' | xargs -0 sed -i 's:tradecraftio/tradecraftd:freicoin/freicoind:g'

# Fix contrib/devtools/copyright_header.py
sed -i 's:Bitcoin Core:Freicoin:g' contrib/devtools/copyright_header.py
sed -i 's:Bitcoin:Freicoin:g' contrib/devtools/copyright_header.py

git checkout -- contrib/devtools/rebrand.sh
git checkout -- contrib/devtools/relicensefile.py
git checkout -- COPYING
git checkout -- depends/Makefile
git checkout -- depends/packages
git checkout -- doc/assets-attribution.md
git checkout -- doc/release-notes
git checkout -- src/test/crypto_tests.cpp
git checkout -- src/bench/data/block413567.raw
sed -i 's:test/test_bitcoin:test/test_freicoin:g' src/test/crypto_tests.cpp
git commit -a -m '[Branding] Re-brand "bitcoin" -> "freicoin".'

# End of File
