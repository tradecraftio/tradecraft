dnl Copyright (c) 2013-2014 The Bitcoin Core developers
dnl Copyright (c) 2013-2023 The Freicoin Developers
dnl
dnl This program is free software: you can redistribute it and/or modify it
dnl under the terms of version 3 of the GNU Affero General Public License as
dnl published by the Free Software Foundation.
dnl
dnl This program is distributed in the hope that it will be useful, but WITHOUT
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
dnl FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
dnl for more details.
dnl
dnl You should have received a copy of the GNU Affero General Public License
dnl along with this program.  If not, see <https://www.gnu.org/licenses/>.

dnl FREICOIN_SUBDIR_TO_INCLUDE([CPPFLAGS-VARIABLE-NAME],[SUBDIRECTORY-NAME],[HEADER-FILE])
dnl SUBDIRECTORY-NAME must end with a path separator
AC_DEFUN([FREICOIN_SUBDIR_TO_INCLUDE],[
  if test "$2" = ""; then
    AC_MSG_RESULT([default])
  else
    echo "#include <$2$3.h>" >conftest.cpp
    newinclpath=`${CXXCPP} ${CPPFLAGS} -M conftest.cpp 2>/dev/null | [ tr -d '\\n\\r\\\\' | sed -e 's/^.*[[:space:]:]\(\/[^[:space:]]*\)]$3[\.h[[:space:]].*$/\1/' -e t -e d`]
    AC_MSG_RESULT([${newinclpath}])
    if test "${newinclpath}" != ""; then
      eval "$1=\"\$$1\"' -I${newinclpath}'"
    fi
  fi
])
