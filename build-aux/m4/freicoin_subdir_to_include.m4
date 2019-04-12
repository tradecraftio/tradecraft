dnl Copyright (c) 2013-2014 The Bitcoin Core developers
dnl Copyright (c) 2013-2019 The Freicoin Developers
dnl
dnl This program is free software: you can redistribute it and/or
dnl modify it under the conjunctive terms of BOTH version 3 of the GNU
dnl Affero General Public License as published by the Free Software
dnl Foundation AND the MIT/X11 software license.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Affero General Public License and the MIT/X11 software license for
dnl more details.
dnl
dnl You should have received a copy of both licenses along with this
dnl program.  If not, see <https://www.gnu.org/licenses/> and
dnl <http://www.opensource.org/licenses/mit-license.php>

dnl FREICOIN_SUBDIR_TO_INCLUDE([CPPFLAGS-VARIABLE-NAME],[SUBDIRECTORY-NAME],[HEADER-FILE])
dnl SUBDIRECTORY-NAME must end with a path separator
AC_DEFUN([FREICOIN_SUBDIR_TO_INCLUDE],[
  if test "x$2" = "x"; then
    AC_MSG_RESULT([default])
  else
    echo "#include <$2$3.h>" >conftest.cpp
    newinclpath=`${CXXCPP} ${CPPFLAGS} -M conftest.cpp 2>/dev/null | [ tr -d '\\n\\r\\\\' | sed -e 's/^.*[[:space:]:]\(\/[^[:space:]]*\)]$3[\.h[[:space:]].*$/\1/' -e t -e d`]
    AC_MSG_RESULT([${newinclpath}])
    if test "x${newinclpath}" != "x"; then
      eval "$1=\"\$$1\"' -I${newinclpath}'"
    fi
  fi
])
