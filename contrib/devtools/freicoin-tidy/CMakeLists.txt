cmake_minimum_required(VERSION 3.22)

project(freicoin-tidy
    VERSION
    1.0.0
    DESCRIPTION "clang-tidy checks for Freicoin"
    LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

set(CMAKE_DISABLE_FIND_PACKAGE_CURL ON)
set(CMAKE_DISABLE_FIND_PACKAGE_FFI ON)
set(CMAKE_DISABLE_FIND_PACKAGE_LibEdit ON)
set(CMAKE_DISABLE_FIND_PACKAGE_LibXml2 ON)
set(CMAKE_DISABLE_FIND_PACKAGE_Terminfo ON)
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB ON)
set(CMAKE_DISABLE_FIND_PACKAGE_zstd ON)

find_package(LLVM REQUIRED CONFIG)
find_program(CLANG_TIDY_EXE NAMES "clang-tidy-${LLVM_VERSION_MAJOR}" "clang-tidy" HINTS ${LLVM_TOOLS_BINARY_DIR})
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")

add_library(freicoin-tidy MODULE freicoin-tidy.cpp logprintf.cpp nontrivial-threadlocal.cpp)
target_include_directories(freicoin-tidy SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

# Disable RTTI and exceptions as necessary
if (MSVC)
    target_compile_options(freicoin-tidy PRIVATE /GR-)
else()
    target_compile_options(freicoin-tidy PRIVATE -fno-rtti)
    target_compile_options(freicoin-tidy PRIVATE -fno-exceptions)
endif()

if(CMAKE_HOST_APPLE)
    # ld64 expects no undefined symbols by default
    target_link_options(freicoin-tidy PRIVATE -Wl,-flat_namespace)
    target_link_options(freicoin-tidy PRIVATE -Wl,-undefined -Wl,suppress)
endif()

# Add warnings
if (MSVC)
    target_compile_options(freicoin-tidy PRIVATE /W4)
else()
    target_compile_options(freicoin-tidy PRIVATE -Wall)
    target_compile_options(freicoin-tidy PRIVATE -Wextra)
endif()

if(CMAKE_VERSION VERSION_LESS 3.27)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" "--load=${CMAKE_BINARY_DIR}/${CMAKE_SHARED_MODULE_PREFIX}freicoin-tidy${CMAKE_SHARED_MODULE_SUFFIX}" "-checks=-*,freicoin-*")
else()
    # CLANG_TIDY_COMMAND supports generator expressions as of 3.27
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" "--load=$<TARGET_FILE:freicoin-tidy>" "-checks=-*,freicoin-*")
endif()

# Create a dummy library that runs clang-tidy tests as a side-effect of building
add_library(freicoin-tidy-tests OBJECT EXCLUDE_FROM_ALL example_logprintf.cpp example_nontrivial-threadlocal.cpp)
add_dependencies(freicoin-tidy-tests freicoin-tidy)

set_target_properties(freicoin-tidy-tests PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")


install(TARGETS freicoin-tidy LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
