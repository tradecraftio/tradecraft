dnl Copyright (c) 2022 The Bitcoin Core developers
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

# GCC 8.1 and earlier requires -lstdc++fs
# Clang 8.0.0 (libc++) and earlier requires -lc++fs

m4_define([_CHECK_FILESYSTEM_testbody], [[
  #include <filesystem>

  namespace fs = std::filesystem;

  int main() {
    (void)fs::current_path().root_name();
    return 0;
  }
]])

AC_DEFUN([CHECK_FILESYSTEM], [

  AC_LANG_PUSH(C++)

  AC_MSG_CHECKING([whether std::filesystem can be used without link library])

  AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_FILESYSTEM_testbody])],[
      AC_MSG_RESULT([yes])
    ],[
      AC_MSG_RESULT([no])
      SAVED_LIBS="$LIBS"
      LIBS="$SAVED_LIBS -lstdc++fs"
      AC_MSG_CHECKING([whether std::filesystem needs -lstdc++fs])
      AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_FILESYSTEM_testbody])],[
          AC_MSG_RESULT([yes])
        ],[
          AC_MSG_RESULT([no])
          AC_MSG_CHECKING([whether std::filesystem needs -lc++fs])
          LIBS="$SAVED_LIBS -lc++fs"
          AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_FILESYSTEM_testbody])],[
            AC_MSG_RESULT([yes])
        ],[
            AC_MSG_FAILURE([cannot figure out how to use std::filesystem])
          ])
        ])
    ])

  AC_LANG_POP
])
