# Copyright (c) 2013-2016 The Freicoin developers
# Copyright (c) 2010-2019 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS)
AM_CXXFLAGS = $(HARDENED_CXXFLAGS)
AM_CPPFLAGS = $(HARDENED_CPPFLAGS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

FREICOIN_CONFIG_INCLUDES=-I$(builddir)/config
FREICOIN_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

FREICOIN_INCLUDES += -I$(srcdir)/secp256k1/include
FREICOIN_INCLUDES += $(UNIVALUE_CFLAGS)

LIBFREICOIN_SERVER=libfreicoin_server.a
LIBFREICOIN_COMMON=libfreicoin_common.a
LIBFREICOIN_CONSENSUS=libfreicoin_consensus.a
LIBFREICOIN_CLI=libfreicoin_cli.a
LIBFREICOIN_UTIL=libfreicoin_util.a
LIBFREICOIN_CRYPTO=crypto/libfreicoin_crypto.a
LIBFREICOINQT=qt/libfreicoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

if ENABLE_ZMQ
LIBFREICOIN_ZMQ=libfreicoin_zmq.a
endif
if BUILD_FREICOIN_LIBS
LIBFREICOINCONSENSUS=libfreicoinconsensus.la
endif
if ENABLE_WALLET
LIBFREICOIN_WALLET=libfreicoin_wallet.a
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBFREICOIN_CRYPTO) \
  $(LIBFREICOIN_UTIL) \
  $(LIBFREICOIN_COMMON) \
  $(LIBFREICOIN_CONSENSUS) \
  $(LIBFREICOIN_SERVER) \
  $(LIBFREICOIN_CLI) \
  $(LIBFREICOIN_WALLET) \
  $(LIBFREICOIN_ZMQ)

lib_LTLIBRARIES = $(LIBFREICOINCONSENSUS)

bin_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_FREICOIND
  bin_PROGRAMS += freicoind
endif

if BUILD_FREICOIN_UTILS
  bin_PROGRAMS += freicoin-cli freicoin-tx
endif

.PHONY: FORCE check-symbols check-security
# freicoin #
FREICOIN_CORE_H = \
  addrman.h \
  base58.h \
  bloom.h \
  blockencodings.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  core_io.h \
  core_memusage.h \
  httprpc.h \
  httpserver.h \
  indirectmap.h \
  init.h \
  key.h \
  keystore.h \
  dbwrapper.h \
  limitedmap.h \
  main.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  netbase.h \
  noui.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  pow.h \
  protocol.h \
  random.h \
  reverselock.h \
  rpc/client.h \
  rpc/protocol.h \
  rpc/server.h \
  rpc/register.h \
  scheduler.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  script/ismine.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/pagelocker.h \
  sync.h \
  threadsafety.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  undo.h \
  util.h \
  utilmoneystr.h \
  utiltime.h \
  validationinterface.h \
  versionbits.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/rpcwallet.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
libfreicoin_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between freicoind and freicoin-qt
libfreicoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libfreicoin_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_server_a_SOURCES = \
  addrman.cpp \
  bloom.cpp \
  blockencodings.cpp \
  chain.cpp \
  checkpoints.cpp \
  httprpc.cpp \
  httpserver.cpp \
  init.cpp \
  dbwrapper.cpp \
  main.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/policy.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  script/sigcache.cpp \
  script/ismine.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  ui_interface.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  $(FREICOIN_CORE_H)

if ENABLE_ZMQ
libfreicoin_zmq_a_CPPFLAGS = $(FREICOIN_INCLUDES) $(ZMQ_CFLAGS)
libfreicoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp
endif


# wallet: shared between freicoind and freicoin-qt, but only linked
# when wallet enabled
libfreicoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_wallet_a_SOURCES = \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  policy/rbf.cpp \
  $(FREICOIN_CORE_H)

# crypto primitives library
crypto_libfreicoin_crypto_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_CONFIG_INCLUDES)
crypto_libfreicoin_crypto_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libfreicoin_crypto_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/common.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.cpp \
  crypto/sha512.h

# consensus: shared between all executables that validate any consensus rules.
libfreicoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/merkleproof.cpp \
  consensus/merkleproof.h \
  consensus/params.h \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/freicoinconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  serialize.h \
  support/cleanse.cpp \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  utilstrencodings.cpp \
  utilstrencodings.h \
  version.h

# common: shared between freicoind, and freicoin-qt and non-server tools
libfreicoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_common_a_SOURCES = \
  amount.cpp \
  base58.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  key.cpp \
  keystore.cpp \
  netbase.cpp \
  protocol.cpp \
  scheduler.cpp \
  script/sign.cpp \
  script/standard.cpp \
  $(FREICOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libfreicoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_util_a_SOURCES = \
  support/pagelocker.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  random.cpp \
  rpc/protocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  util.cpp \
  utilmoneystr.cpp \
  utilstrencodings.cpp \
  utiltime.cpp \
  $(FREICOIN_CORE_H)

if GLIBC_BACK_COMPAT
libfreicoin_util_a_SOURCES += compat/glibc_compat.cpp
endif

# cli: shared between freicoin-cli and freicoin-qt
libfreicoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_cli_a_SOURCES = \
  rpc/client.cpp \
  $(FREICOIN_CORE_H)

nodist_libfreicoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# freicoind binary #
freicoind_SOURCES = freicoind.cpp
freicoind_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
freicoind_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
freicoind_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
freicoind_SOURCES += freicoind-res.rc
endif

freicoind_LDADD = \
  $(LIBFREICOIN_SERVER) \
  $(LIBFREICOIN_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBFREICOIN_UTIL) \
  $(LIBFREICOIN_WALLET) \
  $(LIBFREICOIN_ZMQ) \
  $(LIBFREICOIN_CONSENSUS) \
  $(LIBFREICOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

freicoind_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS)

# freicoin-cli binary #
freicoin_cli_SOURCES = freicoin-cli.cpp
freicoin_cli_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES) $(EVENT_CFLAGS)
freicoin_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
freicoin_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
freicoin_cli_SOURCES += freicoin-cli-res.rc
endif

freicoin_cli_LDADD = \
  $(LIBFREICOIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBFREICOIN_UTIL) \
  $(LIBFREICOIN_CRYPTO)

freicoin_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)
#

# freicoin-tx binary #
freicoin_tx_SOURCES = freicoin-tx.cpp
freicoin_tx_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
freicoin_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
freicoin_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
freicoin_tx_SOURCES += freicoin-tx-res.rc
endif

freicoin_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBFREICOIN_COMMON) \
  $(LIBFREICOIN_UTIL) \
  $(LIBFREICOIN_CONSENSUS) \
  $(LIBFREICOIN_CRYPTO) \
  $(LIBSECP256K1)

freicoin_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# freicoinconsensus library #
if BUILD_FREICOIN_LIBS
include_HEADERS = script/freicoinconsensus.h
libfreicoinconsensus_la_SOURCES = $(crypto_libfreicoin_crypto_a_SOURCES) $(libfreicoin_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libfreicoinconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libfreicoinconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libfreicoinconsensus_la_LIBADD = $(LIBSECP256K1)
libfreicoinconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_FREICOIN_INTERNAL
libfreicoinconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = $(CTAES_DIST)

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

.mm.o:
	$(AM_V_CXX) $(OBJCXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(AM_CXXFLAGS) $(PIE_FLAGS) $(CXXFLAGS) -c -o $@ $<

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(<D) $<

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
