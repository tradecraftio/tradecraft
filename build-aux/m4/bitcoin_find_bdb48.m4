dnl Copyright (c) 2013-2015 The Bitcoin Core developers
dnl Copyright (c) 2013-2021 The Freicoin Developers
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

AC_DEFUN([BITCOIN_FIND_BDB48],[
  AC_ARG_VAR(BDB_CFLAGS, [C compiler flags for BerkeleyDB, bypasses autodetection])
  AC_ARG_VAR(BDB_LIBS, [Linker flags for BerkeleyDB, bypasses autodetection])

  if test "x$BDB_CFLAGS" = "x"; then
    AC_MSG_CHECKING([for Berkeley DB C++ headers])
    BDB_CPPFLAGS=
    bdbpath=X
    bdb48path=X
    bdbdirlist=
    for _vn in 4.8 48 4 5 ''; do
      for _pfx in b lib ''; do
        bdbdirlist="$bdbdirlist ${_pfx}db${_vn}"
      done
    done
    for searchpath in $bdbdirlist ''; do
      test -n "${searchpath}" && searchpath="${searchpath}/"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <${searchpath}db_cxx.h>
      ]],[[
        #if !((DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8) || DB_VERSION_MAJOR > 4)
          #error "failed to find bdb 4.8+"
        #endif
      ]])],[
        if test "x$bdbpath" = "xX"; then
          bdbpath="${searchpath}"
        fi
      ],[
        continue
      ])
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <${searchpath}db_cxx.h>
      ]],[[
        #if !(DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR == 8)
          #error "failed to find bdb 4.8"
        #endif
      ]])],[
        bdb48path="${searchpath}"
        break
      ],[])
    done
    if test "x$bdbpath" = "xX"; then
      AC_MSG_RESULT([no])
      AC_MSG_ERROR([libdb_cxx headers missing, ]AC_PACKAGE_NAME[ requires this library for wallet functionality (--disable-wallet to disable wallet functionality)])
    elif test "x$bdb48path" = "xX"; then
      BITCOIN_SUBDIR_TO_INCLUDE(BDB_CPPFLAGS,[${bdbpath}],db_cxx)
      AC_ARG_WITH([incompatible-bdb],[AS_HELP_STRING([--with-incompatible-bdb], [allow using a bdb version other than 4.8])],[
        AC_MSG_WARN([Found Berkeley DB other than 4.8; wallets opened by this build will not be portable!])
      ],[
        AC_MSG_ERROR([Found Berkeley DB other than 4.8, required for portable wallets (--with-incompatible-bdb to ignore or --disable-wallet to disable wallet functionality)])
      ])
    else
      BITCOIN_SUBDIR_TO_INCLUDE(BDB_CPPFLAGS,[${bdb48path}],db_cxx)
      bdbpath="${bdb48path}"
    fi
  else
    BDB_CPPFLAGS=${BDB_CFLAGS}
  fi
  AC_SUBST(BDB_CPPFLAGS)
  
  if test "x$BDB_LIBS" = "x"; then
    # TODO: Ideally this could find the library version and make sure it matches the headers being used
    for searchlib in db_cxx-4.8 db_cxx; do
      AC_CHECK_LIB([$searchlib],[main],[
        BDB_LIBS="-l${searchlib}"
        break
      ])
    done
    if test "x$BDB_LIBS" = "x"; then
        AC_MSG_ERROR([libdb_cxx missing, ]AC_PACKAGE_NAME[ requires this library for wallet functionality (--disable-wallet to disable wallet functionality)])
    fi
  fi
  AC_SUBST(BDB_LIBS)
])
