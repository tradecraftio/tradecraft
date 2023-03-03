# Copyright (c) 2013-2016 The Bitcoin Core developers
# Copyright (c) 2010-2023 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of version 3 of the GNU Affero General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
# details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%: FORCE
	@echo '$*'='$($*)'

DIST_SUBDIRS = secp256k1

AM_LDFLAGS = $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS) $(LTO_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS) $(LTO_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)
EXTRA_LIBRARIES =

FREICOIN_INCLUDES=-I$(builddir) -I$(srcdir)/$(MINISKETCH_INCLUDE_DIR_INT) -I$(srcdir)/secp256k1/include -I$(srcdir)/$(UNIVALUE_INCLUDE_DIR_INT) $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS)

LIBFREICOIN_NODE=libfreicoin_node.a
LIBFREICOIN_COMMON=libfreicoin_common.a
LIBFREICOIN_CONSENSUS=libfreicoin_consensus.a
LIBFREICOIN_CLI=libfreicoin_cli.a
LIBFREICOIN_UTIL=libfreicoin_util.a
LIBFREICOIN_CRYPTO_BASE=crypto/libfreicoin_crypto_base.a
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
LIBFREICOIN_WALLET_TOOL=libfreicoin_wallet_tool.a
endif

LIBFREICOIN_CRYPTO= $(LIBFREICOIN_CRYPTO_BASE)
if ENABLE_SSE41
LIBFREICOIN_CRYPTO_SSE41 = crypto/libfreicoin_crypto_sse41.a
LIBFREICOIN_CRYPTO += $(LIBFREICOIN_CRYPTO_SSE41)
endif
if ENABLE_AVX2
LIBFREICOIN_CRYPTO_AVX2 = crypto/libfreicoin_crypto_avx2.a
LIBFREICOIN_CRYPTO += $(LIBFREICOIN_CRYPTO_AVX2)
endif
if ENABLE_X86_SHANI
LIBFREICOIN_CRYPTO_X86_SHANI = crypto/libfreicoin_crypto_x86_shani.a
LIBFREICOIN_CRYPTO += $(LIBFREICOIN_CRYPTO_X86_SHANI)
endif
if ENABLE_ARM_SHANI
LIBFREICOIN_CRYPTO_ARM_SHANI = crypto/libfreicoin_crypto_arm_shani.a
LIBFREICOIN_CRYPTO += $(LIBFREICOIN_CRYPTO_ARM_SHANI)
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBFREICOIN_CRYPTO) \
  $(LIBFREICOIN_UTIL) \
  $(LIBFREICOIN_COMMON) \
  $(LIBFREICOIN_CONSENSUS) \
  $(LIBFREICOIN_NODE) \
  $(LIBFREICOIN_CLI) \
  $(LIBFREICOIN_IPC) \
  $(LIBFREICOIN_WALLET) \
  $(LIBFREICOIN_WALLET_TOOL) \
  $(LIBFREICOIN_ZMQ)

lib_LTLIBRARIES = $(LIBFREICOINCONSENSUS)
noinst_LTLIBRARIES =

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_FREICOIND
  bin_PROGRAMS += freicoind
endif

if BUILD_FREICOIN_NODE
  bin_PROGRAMS += freicoin-node
endif

if BUILD_FREICOIN_CLI
  bin_PROGRAMS += freicoin-cli
endif

if BUILD_FREICOIN_TX
  bin_PROGRAMS += freicoin-tx
endif

if ENABLE_WALLET
if BUILD_FREICOIN_WALLET
  bin_PROGRAMS += freicoin-wallet
endif
endif

if BUILD_FREICOIN_UTIL
  bin_PROGRAMS += freicoin-util
endif

.PHONY: FORCE check-symbols check-security
# freicoin #
FREICOIN_CORE_H = \
  addrdb.h \
  addrman.h \
  addrman_impl.h \
  attributes.h \
  banman.h \
  base58.h \
  bech32.h \
  blockencodings.h \
  blockfilter.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  common/bloom.h \
  compat.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/cpuid.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/merkleproof.h \
  consensus/tx_check.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  dbwrapper.h \
  deploymentinfo.h \
  deploymentstatus.h \
  external_signer.h \
  flatfile.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  i2p.h \
  index/base.h \
  index/blockfilterindex.h \
  index/coinstatsindex.h \
  index/disktxpos.h \
  index/txindex.h \
  indirectmap.h \
  init.h \
  init/common.h \
  interfaces/chain.h \
  interfaces/echo.h \
  interfaces/handler.h \
  interfaces/init.h \
  interfaces/ipc.h \
  interfaces/node.h \
  interfaces/wallet.h \
  key.h \
  key_io.h \
  logging.h \
  logging/timer.h \
  mapport.h \
  memusage.h \
  merkleblock.h \
  net.h \
  net_permissions.h \
  net_processing.h \
  net_types.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  node/blockstorage.h \
  node/caches.h \
  node/chainstate.h \
  node/coin.h \
  node/coinstats.h \
  node/context.h \
  node/miner.h \
  node/minisketchwrapper.h \
  node/pst.h \
  node/transaction.h \
  node/ui_interface.h \
  node/utxo_snapshot.h \
  noui.h \
  outputtype.h \
  policy/feerate.h \
  policy/fees.h \
  policy/packages.h \
  policy/policy.h \
  policy/rbf.h \
  policy/settings.h \
  pow.h \
  protocol.h \
  pst.h \
  random.h \
  randomenv.h \
  reverse_iterator.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/rawtransaction_util.h \
  rpc/register.h \
  rpc/request.h \
  rpc/server.h \
  rpc/server_util.h \
  rpc/util.h \
  scheduler.h \
  script/descriptor.h \
  script/keyorigin.h \
  script/sigcache.h \
  script/sign.h \
  script/signingprovider.h \
  script/standard.h \
  shutdown.h \
  signet.h \
  stratum.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadinterrupt.h \
  threadsafety.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  txorphanage.h \
  txrequest.h \
  undo.h \
  util/asmap.h \
  util/bip32.h \
  util/bytevectorhash.h \
  util/check.h \
  util/epochguard.h \
  util/error.h \
  util/fastrange.h \
  util/fees.h \
  util/getuniquepath.h \
  util/golombrice.h \
  util/hash_type.h \
  util/hasher.h \
  util/macros.h \
  util/message.h \
  util/moneystr.h \
  util/overflow.h \
  util/overloaded.h \
  util/readwritefile.h \
  util/serfloat.h \
  util/settings.h \
  util/sock.h \
  util/spanparsing.h \
  util/string.h \
  util/syscall_sandbox.h \
  util/system.h \
  util/thread.h \
  util/threadnames.h \
  util/time.h \
  util/tokenpipe.h \
  util/trace.h \
  util/translation.h \
  util/types.h \
  util/ui_change_type.h \
  util/url.h \
  util/vector.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  wallet/bdb.h \
  wallet/coincontrol.h \
  wallet/coinselection.h \
  wallet/context.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/dump.h \
  wallet/external_signer_scriptpubkeyman.h \
  wallet/feebumper.h \
  wallet/fees.h \
  wallet/ismine.h \
  wallet/load.h \
  wallet/miner.h \
  wallet/receive.h \
  wallet/rpc/util.h \
  wallet/rpc/wallet.h \
  wallet/salvage.h \
  wallet/scriptpubkeyman.h \
  wallet/spend.h \
  wallet/sqlite.h \
  wallet/transaction.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/wallettool.h \
  wallet/walletutil.h \
  walletinitinterface.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  zmq/zmqrpc.h \
  zmq/zmqutil.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libfreicoin_util_a-clientversion.$(OBJEXT): obj/build.h

ipc/capnp/libfreicoin_ipc_a-ipc.$(OBJEXT): $(libfreicoin_ipc_mpgen_input:=.h)

# server: shared between freicoind and freicoin-qt
# Contains code accessing mempool and chain state that is meant to be separated
# from wallet and gui code (see node/README.md). Shared code should go in
# libfreicoin_common or libfreicoin_util libraries, instead.
libfreicoin_node_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(NATPMP_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libfreicoin_node_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_node_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  banman.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  chain.cpp \
  consensus/tx_verify.cpp \
  dbwrapper.cpp \
  deploymentstatus.cpp \
  flatfile.cpp \
  httprpc.cpp \
  httpserver.cpp \
  i2p.cpp \
  index/base.cpp \
  index/blockfilterindex.cpp \
  index/coinstatsindex.cpp \
  index/txindex.cpp \
  init.cpp \
  mapport.cpp \
  net.cpp \
  net_processing.cpp \
  node/blockstorage.cpp \
  node/caches.cpp \
  node/chainstate.cpp \
  node/coin.cpp \
  node/coinstats.cpp \
  node/context.cpp \
  node/interfaces.cpp \
  node/miner.cpp \
  node/minisketchwrapper.cpp \
  node/pst.cpp \
  node/transaction.cpp \
  node/ui_interface.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/packages.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  rpc/server_util.cpp \
  script/sigcache.cpp \
  shutdown.cpp \
  signet.cpp \
  stratum.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  txorphanage.cpp \
  txrequest.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  $(FREICOIN_CORE_H)

if ENABLE_WALLET
libfreicoin_node_a_SOURCES += wallet/init.cpp
endif
if !ENABLE_WALLET
libfreicoin_node_a_SOURCES += dummywallet.cpp
endif

if ENABLE_ZMQ
libfreicoin_zmq_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES) $(ZMQ_CFLAGS)
libfreicoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp \
  zmq/zmqrpc.cpp \
  zmq/zmqutil.cpp
endif


# wallet: shared between freicoind and freicoin-qt, but only linked
# when wallet enabled
libfreicoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES) $(SQLITE_CFLAGS)
libfreicoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_wallet_a_SOURCES = \
  wallet/coincontrol.cpp \
  wallet/context.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/dump.cpp \
  wallet/external_signer_scriptpubkeyman.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/interfaces.cpp \
  wallet/load.cpp \
  wallet/miner.cpp \
  wallet/receive.cpp \
  wallet/rpc/addresses.cpp \
  wallet/rpc/backup.cpp \
  wallet/rpc/coins.cpp \
  wallet/rpc/encrypt.cpp \
  wallet/rpc/spend.cpp \
  wallet/rpc/signmessage.cpp \
  wallet/rpc/transactions.cpp \
  wallet/rpc/util.cpp \
  wallet/rpc/wallet.cpp \
  wallet/scriptpubkeyman.cpp \
  wallet/spend.cpp \
  wallet/transaction.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  $(FREICOIN_CORE_H)

if USE_SQLITE
libfreicoin_wallet_a_SOURCES += wallet/sqlite.cpp
endif
if USE_BDB
libfreicoin_wallet_a_SOURCES += wallet/bdb.cpp wallet/salvage.cpp
endif

libfreicoin_wallet_tool_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_wallet_tool_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_wallet_tool_a_SOURCES = \
  wallet/wallettool.cpp \
  $(FREICOIN_CORE_H)

# crypto primitives library
crypto_libfreicoin_crypto_base_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libfreicoin_crypto_base_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libfreicoin_crypto_base_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha_poly_aead.h \
  crypto/chacha_poly_aead.cpp \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hkdf_sha256_32.cpp \
  crypto/hkdf_sha256_32.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/poly1305.h \
  crypto/poly1305.cpp \
  crypto/muhash.h \
  crypto/muhash.cpp \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha3.cpp \
  crypto/sha3.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/siphash.cpp \
  crypto/siphash.h

if USE_ASM
crypto_libfreicoin_crypto_base_a_SOURCES += crypto/sha256_sse4.cpp
endif

crypto_libfreicoin_crypto_sse41_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libfreicoin_crypto_sse41_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libfreicoin_crypto_sse41_a_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_libfreicoin_crypto_sse41_a_CPPFLAGS += -DENABLE_SSE41
crypto_libfreicoin_crypto_sse41_a_SOURCES = crypto/sha256_sse41.cpp

crypto_libfreicoin_crypto_avx2_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libfreicoin_crypto_avx2_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libfreicoin_crypto_avx2_a_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_libfreicoin_crypto_avx2_a_CPPFLAGS += -DENABLE_AVX2
crypto_libfreicoin_crypto_avx2_a_SOURCES = crypto/sha256_avx2.cpp

crypto_libfreicoin_crypto_x86_shani_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libfreicoin_crypto_x86_shani_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libfreicoin_crypto_x86_shani_a_CXXFLAGS += $(X86_SHANI_CXXFLAGS)
crypto_libfreicoin_crypto_x86_shani_a_CPPFLAGS += -DENABLE_X86_SHANI
crypto_libfreicoin_crypto_x86_shani_a_SOURCES = crypto/sha256_x86_shani.cpp

crypto_libfreicoin_crypto_arm_shani_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libfreicoin_crypto_arm_shani_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libfreicoin_crypto_arm_shani_a_CXXFLAGS += $(ARM_SHANI_CXXFLAGS)
crypto_libfreicoin_crypto_arm_shani_a_CPPFLAGS += -DENABLE_ARM_SHANI
crypto_libfreicoin_crypto_arm_shani_a_SOURCES = crypto/sha256_arm_shani.cpp

# consensus: shared between all executables that validate any consensus rules.
libfreicoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_consensus_a_SOURCES = \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/amount.h \
  consensus/amount.cpp \
  consensus/merkle.cpp \
  consensus/merkleproof.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/tx_check.cpp \
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
  span.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  util/strencodings.cpp \
  util/strencodings.h \
  version.h

# common: shared between freicoind, and freicoin-qt and non-server tools
libfreicoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_common_a_SOURCES = \
  base58.cpp \
  bech32.cpp \
  chainparams.cpp \
  coins.cpp \
  common/bloom.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  deploymentinfo.cpp \
  external_signer.cpp \
  init/common.cpp \
  key.cpp \
  key_io.cpp \
  merkleblock.cpp \
  net_types.cpp \
  netaddress.cpp \
  netbase.cpp \
  net_permissions.cpp \
  outputtype.cpp \
  policy/feerate.cpp \
  policy/policy.cpp \
  protocol.cpp \
  pst.cpp \
  rpc/rawtransaction_util.cpp \
  rpc/external_signer.cpp \
  rpc/util.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/sign.cpp \
  script/signingprovider.cpp \
  script/standard.cpp \
  warnings.cpp \
  $(FREICOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# sanity checks are linked.
libfreicoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  interfaces/echo.cpp \
  interfaces/handler.cpp \
  interfaces/init.cpp \
  logging.cpp \
  random.cpp \
  randomenv.cpp \
  rpc/request.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util/asmap.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/error.cpp \
  util/fees.cpp \
  util/getuniquepath.cpp \
  util/hasher.cpp \
  util/sock.cpp \
  util/system.cpp \
  util/message.cpp \
  util/moneystr.cpp \
  util/readwritefile.cpp \
  util/settings.cpp \
  util/thread.cpp \
  util/threadnames.cpp \
  util/serfloat.cpp \
  util/spanparsing.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/syscall_sandbox.cpp \
  util/time.cpp \
  util/tokenpipe.cpp \
  $(FREICOIN_CORE_H)

if USE_LIBEVENT
libfreicoin_util_a_SOURCES += util/url.cpp
endif

# cli: shared between freicoin-cli and freicoin-qt
libfreicoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfreicoin_cli_a_SOURCES = \
  compat/stdin.h \
  compat/stdin.cpp \
  rpc/client.cpp \
  $(FREICOIN_CORE_H)

nodist_libfreicoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# freicoind & freicoin-node binaries #
freicoin_daemon_sources = freicoind.cpp
freicoin_bin_cppflags = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
freicoin_bin_cxxflags = $(AM_CXXFLAGS) $(PIE_FLAGS)
freicoin_bin_ldflags = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
freicoin_daemon_sources += freicoind-res.rc
endif

freicoin_bin_ldadd = \
  $(LIBFREICOIN_WALLET) \
  $(LIBFREICOIN_COMMON) \
  $(LIBFREICOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBFREICOIN_ZMQ) \
  $(LIBFREICOIN_CONSENSUS) \
  $(LIBFREICOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

freicoin_bin_ldadd += $(BDB_LIBS) $(MINIUPNPC_LIBS) $(NATPMP_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(SQLITE_LIBS)

freicoind_SOURCES = $(freicoin_daemon_sources) init/freicoind.cpp
freicoind_CPPFLAGS = $(freicoin_bin_cppflags)
freicoind_CXXFLAGS = $(freicoin_bin_cxxflags)
freicoind_LDFLAGS = $(freicoin_bin_ldflags)
freicoind_LDADD = $(LIBFREICOIN_NODE) $(freicoin_bin_ldadd)

freicoin_node_SOURCES = $(freicoin_daemon_sources) init/freicoin-node.cpp
freicoin_node_CPPFLAGS = $(freicoin_bin_cppflags)
freicoin_node_CXXFLAGS = $(freicoin_bin_cxxflags)
freicoin_node_LDFLAGS = $(freicoin_bin_ldflags)
freicoin_node_LDADD = $(LIBFREICOIN_NODE) $(freicoin_bin_ldadd) $(LIBFREICOIN_IPC) $(LIBMULTIPROCESS_LIBS)

# freicoin-cli binary #
freicoin_cli_SOURCES = freicoin-cli.cpp
freicoin_cli_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES) $(EVENT_CFLAGS)
freicoin_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
freicoin_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
freicoin_cli_SOURCES += freicoin-cli-res.rc
endif

freicoin_cli_LDADD = \
  $(LIBFREICOIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBFREICOIN_UTIL) \
  $(LIBFREICOIN_CRYPTO)

freicoin_cli_LDADD += $(EVENT_LIBS)
#

# freicoin-tx binary #
freicoin_tx_SOURCES = freicoin-tx.cpp
freicoin_tx_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
freicoin_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
freicoin_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

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
#

# freicoin-wallet binary #
freicoin_wallet_SOURCES = freicoin-wallet.cpp
freicoin_wallet_SOURCES += init/freicoin-wallet.cpp
freicoin_wallet_CPPFLAGS = $(freicoin_bin_cppflags)
freicoin_wallet_CXXFLAGS = $(freicoin_bin_cxxflags)
freicoin_wallet_LDFLAGS = $(freicoin_bin_ldflags)
freicoin_wallet_LDADD = \
  $(LIBFREICOIN_WALLET_TOOL) \
  $(LIBFREICOIN_WALLET) \
  $(LIBFREICOIN_COMMON) \
  $(LIBFREICOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBFREICOIN_CONSENSUS) \
  $(LIBFREICOIN_CRYPTO) \
  $(LIBSECP256K1) \
  $(BDB_LIBS) \
  $(SQLITE_LIBS)

if TARGET_WINDOWS
freicoin_wallet_SOURCES += freicoin-wallet-res.rc
endif
#

# freicoin-util binary #
freicoin_util_SOURCES = freicoin-util.cpp
freicoin_util_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
freicoin_util_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
freicoin_util_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
freicoin_util_SOURCES += freicoin-util-res.rc
endif

freicoin_util_LDADD = \
  $(LIBFREICOIN_COMMON) \
  $(LIBFREICOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBFREICOIN_CONSENSUS) \
  $(LIBFREICOIN_CRYPTO) \
  $(LIBSECP256K1)
#

# freicoinconsensus library #
if BUILD_FREICOIN_LIBS
include_HEADERS = script/freicoinconsensus.h
libfreicoinconsensus_la_SOURCES = support/cleanse.cpp $(crypto_libfreicoin_crypto_base_a_SOURCES) $(libfreicoin_consensus_a_SOURCES)

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
CLEANFILES += crc32c/src/*.gcda crc32c/src/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += index/*.gcda index/*.gcno
CLEANFILES += interfaces/*.gcda interfaces/*.gcno
CLEANFILES += node/*.gcda node/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += rpc/*.gcda rpc/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += util/*.gcda util/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST = $(CTAES_DIST)


config/freicoin-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/freicoin-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/freicoin-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/freicoin-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
	@echo "Running symbol and dynamic library checks..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/security-check.py $(bin_PROGRAMS)
endif

libfreicoin_ipc_mpgen_input = \
  ipc/capnp/echo.capnp \
  ipc/capnp/init.capnp
EXTRA_DIST += $(libfreicoin_ipc_mpgen_input)
%.capnp:

if BUILD_MULTIPROCESS
LIBFREICOIN_IPC=libfreicoin_ipc.a
libfreicoin_ipc_a_SOURCES = \
  ipc/capnp/context.h \
  ipc/capnp/init-types.h \
  ipc/capnp/protocol.cpp \
  ipc/capnp/protocol.h \
  ipc/context.h \
  ipc/exception.h \
  ipc/interfaces.cpp \
  ipc/process.cpp \
  ipc/process.h \
  ipc/protocol.h
libfreicoin_ipc_a_CPPFLAGS = $(AM_CPPFLAGS) $(FREICOIN_INCLUDES)
libfreicoin_ipc_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) $(LIBMULTIPROCESS_CFLAGS)

include $(MPGEN_PREFIX)/include/mpgen.mk
libfreicoin_ipc_mpgen_output = \
  $(libfreicoin_ipc_mpgen_input:=.c++) \
  $(libfreicoin_ipc_mpgen_input:=.h) \
  $(libfreicoin_ipc_mpgen_input:=.proxy-client.c++) \
  $(libfreicoin_ipc_mpgen_input:=.proxy-server.c++) \
  $(libfreicoin_ipc_mpgen_input:=.proxy-types.c++) \
  $(libfreicoin_ipc_mpgen_input:=.proxy-types.h) \
  $(libfreicoin_ipc_mpgen_input:=.proxy.h)
nodist_libfreicoin_ipc_a_SOURCES = $(libfreicoin_ipc_mpgen_output)
CLEANFILES += $(libfreicoin_ipc_mpgen_output)
endif

include Makefile.minisketch.include

include Makefile.crc32c.include
include Makefile.leveldb.include

include Makefile.test_util.include
include Makefile.test_fuzz.include

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

include Makefile.univalue.include
