#!/bin/bash
#
# build-toolchain.sh — build the period dependencies needed to compile the
# Bitcoin 0.1.0/0.1.3 GUI from source with a MinGW cross-compiler.
#
# It fetches and cross-builds, into ./toolchain/ :
#   * Berkeley DB 4.8.30.NC   (libdb_cxx)   — the wallet/blockchain database
#   * wxWidgets 2.8.12 (ANSI, static)       — the GUI toolkit
#   * Boost 1.55 (headers only)             — foreach / lexical_cast / tuple
#   * libeay32.a                            — import lib for Satoshi's OpenSSL
#                                             0.9.8 DLL (../../bitcoin-nov08-rebuild)
#
# Prerequisites (Debian/Ubuntu):
#   sudo apt install g++-mingw-w64-i686 mingw-w64-tools curl
#
# Run once, then use ./build.sh <version> <network>.  ~110 MB download, ~15 min.
set -e
HOST=i686-w64-mingw32
HERE="$(cd "$(dirname "$0")" && pwd)"
TC="$HERE/toolchain"
SRC="$TC/src"
INST="$TC/inst"
mkdir -p "$SRC" "$INST"
JOBS=$(nproc 2>/dev/null || echo 4)

fetch() { # url file
    [ -f "$SRC/$2" ] || curl -L "$1" -o "$SRC/$2"
}

echo "== downloading sources =="
fetch "https://github.com/wxWidgets/wxWidgets/releases/download/v2.8.12/wxWidgets-2.8.12.tar.gz" wx.tar.gz
fetch "https://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz" db.tar.gz
fetch "https://archives.boost.io/release/1.55.0/source/boost_1_55_0.tar.gz" boost.tar.gz

echo "== Boost (headers only) =="
[ -d "$SRC/boost_1_55_0" ] || tar xzf "$SRC/boost.tar.gz" -C "$SRC"

echo "== Berkeley DB 4.8 (cross-MinGW) =="
if [ ! -f "$TC/db/build_unix/libdb_cxx-4.8.a" ]; then
    rm -rf "$TC/db"; tar xzf "$SRC/db.tar.gz" -C "$TC"; mv "$TC/db-4.8.30.NC" "$TC/db"
    # known fix: BDB 4.8 redefines __atomic_compare_exchange (a GCC builtin)
    sed -i 's/__atomic_compare_exchange/__atomic_compare_exchange_db/g' "$TC/db/dbinc/atomic.h"
    ( cd "$TC/db/build_unix" && ../dist/configure --enable-mingw --enable-cxx \
        --disable-replication --disable-shared --enable-static --host=$HOST \
        CC=$HOST-gcc CXX=$HOST-g++ >/dev/null && make -j"$JOBS" libdb_cxx-4.8.a >/dev/null )
fi

echo "== wxWidgets 2.8.12 (ANSI, static, cross-MinGW) =="
if [ ! -f "$TC/wx/build/lib/libwx_base-2.8-$HOST.a" ]; then
    rm -rf "$TC/wx"; tar xzf "$SRC/wx.tar.gz" -C "$TC"; mv "$TC/wxWidgets-2.8.12" "$TC/wx"
    # build fix: MinGW-w64 + modern GCC -> _mkdir/_rmdir live in <direct.h>
    sed -i 's#\(#include "wx/arrstr.h"\)#\1\n#ifdef __MINGW32__\n#include <direct.h>\n#endif#' "$TC/wx/include/wx/filefn.h"
    mkdir -p "$TC/wx/build"
    ( cd "$TC/wx/build" && ../configure --host=$HOST --build="$(../config.guess)" \
        --disable-shared --enable-static --disable-unicode --disable-debug \
        --enable-richtext --enable-html --with-regex=builtin \
        --with-libpng=builtin --with-libjpeg=builtin --with-libtiff=builtin \
        --with-zlib=builtin --with-expat=builtin \
        --disable-mediactrl --disable-sound --disable-mshtmlhelp \
        CXXFLAGS="-std=gnu++98 -fpermissive -w" CFLAGS="-w" >/dev/null \
      && make -j"$JOBS" >/dev/null 2>&1 || true )   # samples may fail; we only need the libs
fi

echo "== libeay32.a (import lib for Satoshi's OpenSSL 0.9.8 DLL) =="
VENDOR="$HERE/../../bitcoin-nov08-rebuild/vendor"
if [ ! -f "$VENDOR/libeay32.a" ]; then
    ( cd "$VENDOR" && gendef libeay32.dll \
      && $HOST-dlltool -d libeay32.def -l libeay32.a -D libeay32.dll )
fi

echo "== toolchain ready in $TC =="
