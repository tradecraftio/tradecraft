#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Copyright (c) 2010-2021 The Freicoin Developers
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

export LC_ALL=C.UTF-8

if [ -n "$QEMU_USER_CMD" ]; then
  BEGIN_FOLD wrap-qemu
  # Generate all binaries, so that they can be wrapped
  DOCKER_EXEC make $MAKEJOBS -C src/secp256k1 VERBOSE=1
  DOCKER_EXEC make $MAKEJOBS -C src/univalue VERBOSE=1
  DOCKER_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-qemu.sh"
  END_FOLD
fi

if [ -n "$USE_VALGRIND" ]; then
  BEGIN_FOLD wrap-valgrind
  DOCKER_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-valgrind.sh"
  END_FOLD
fi

bash -c "${CI_WAIT}" &  # Print dots in case the tests take a long time to run

if [ "$RUN_UNIT_TESTS" = "true" ]; then
  BEGIN_FOLD unit-tests
  DOCKER_EXEC LD_LIBRARY_PATH=$DEPENDS_DIR/$HOST/lib make $MAKEJOBS check VERBOSE=1
  END_FOLD
fi

if [ "$RUN_UNIT_TESTS_SEQUENTIAL" = "true" ]; then
  BEGIN_FOLD unit-tests-seq
  DOCKER_EXEC LD_LIBRARY_PATH=$DEPENDS_DIR/$HOST/lib "${BASE_ROOT_DIR}/build/bitcoin-*/src/test/test_bitcoin" --catch_system_errors=no -l test_suite
  END_FOLD
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
  BEGIN_FOLD functional-tests
  DOCKER_EXEC test/functional/test_runner.py --ci $MAKEJOBS --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=4000 ${TEST_RUNNER_EXTRA} --quiet --failfast
  END_FOLD
fi

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  BEGIN_FOLD fuzz-tests
  DOCKER_EXEC test/fuzz/test_runner.py ${FUZZ_TESTS_CONFIG} -l DEBUG ${DIR_FUZZ_IN}
  END_FOLD
fi
