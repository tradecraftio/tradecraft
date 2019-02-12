#!/bin/bash
# Copyright (c) 2014 The Bitcoin Core developers
# Copyright (c) 2010-2019 The Freicoin developers
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
TIMEOUT=10
SIGNAL=HUP
PIDFILE=.send.pid
if [ $# -eq 0 ]; then
  echo -e "Usage:\t$0 <cmd>"
  echo -e "\tRuns <cmd> and wait ${TIMEOUT} seconds or until SIG${SIGNAL} is received."
  echo -e "\tReturns: 0 if SIG${SIGNAL} is received, 1 otherwise."
  echo -e "Or:\t$0 -STOP"
  echo -e "\tsends SIG${SIGNAL} to running send.sh"
  exit 0
fi

if [ $1 = "-STOP" ]; then
  if [ -s ${PIDFILE} ]; then
      kill -s ${SIGNAL} $(<$PIDFILE 2>/dev/null) 2>/dev/null
  fi
  exit 0
fi

trap '[[ ${PID} ]] && kill ${PID}' ${SIGNAL}
trap 'rm -f ${PIDFILE}' EXIT
echo $$ > ${PIDFILE}
"$@"
sleep ${TIMEOUT} & PID=$!
wait ${PID} && exit 1

exit 0
