#!/bin/bash
#
# build-toolchain.sh — build the period dependencies shared by every from-source
# build in this repo (the nov08 rebuild GUI, the 0.1.0/0.1.3 GUI, and the IRC
# clients), with a MinGW cross-compiler.
#
# Committed alongside this script:
#   * openssl/        — the period OpenSSL 0.9.8 headers (so bignum.h/key.h
#                       compile unmodified)
#
# Fetched and cross-built into ./build/ (git-ignored):
#   * Berkeley DB 4.8.30.NC   (libdb_cxx)   — the wallet/blockchain database
#   * wxWidgets 2.8.12 (ANSI, static)       — the GUI toolkit (richtext+html)
#   * Boost 1.55 (headers only)             — foreach / lexical_cast / tuple
#   * libeay32.a                            — import lib for Satoshi's OpenSSL
#                                             0.9.8 DLL (from ../bitcoin-0.1.0)
#
# The wxWidgets / Berkeley DB compilation fixes are applied from
# ../patches/compilation/ (see that folder's README).
#
# Prerequisites (Debian/Ubuntu):
#   sudo apt install g++-mingw-w64-i686 mingw-w64-tools curl
#
# Run once, then `make` in any project folder (bitcoin-0.1.0/src,
# bitcoin-nov08-rebuild, irc/<network>/bitcoin-<version>/).
# ~110 MB download, ~15 min.
set -e
HOST=i686-w64-mingw32
HERE="$(cd "$(dirname "$0")" && pwd)"
TC="$HERE/build"
PATCHES="$HERE/../patches/compilation"
SRC="$TC/src"
mkdir -p "$SRC"
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
    # compilation fix: BDB 4.8 redefines __atomic_compare_exchange (a GCC builtin)
    patch -p1 -d "$TC/db" < "$PATCHES/berkeley-db-4.8-gcc-atomic.patch"
    ( cd "$TC/db/build_unix" && ../dist/configure --enable-mingw --enable-cxx \
        --disable-replication --disable-shared --enable-static --host=$HOST \
        CC=$HOST-gcc CXX=$HOST-g++ >/dev/null && make -j"$JOBS" libdb_cxx-4.8.a >/dev/null )
fi

echo "== wxWidgets 2.8.12 (ANSI, static, cross-MinGW) =="
if [ ! -f "$TC/wx/build/lib/libwx_base-2.8-$HOST.a" ]; then
    rm -rf "$TC/wx"; tar xzf "$SRC/wx.tar.gz" -C "$TC"; mv "$TC/wxWidgets-2.8.12" "$TC/wx"
    # compilation fix: MinGW-w64 + modern GCC -> _mkdir/_rmdir live in <direct.h>
    # (the narrowing issue is avoided by building in -std=gnu++98 below, no flag)
    patch -p1 -d "$TC/wx" < "$PATCHES/wxWidgets-2.8.12-mingw.patch"
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
if [ ! -f "$TC/libeay32.a" ]; then
    cp -f "$HERE/../bitcoin-0.1.0/libeay32.dll" "$TC/libeay32.dll"
    ( cd "$TC" && gendef libeay32.dll \
      && $HOST-dlltool -d libeay32.def -l libeay32.a -D libeay32.dll )
fi

echo "== toolchain ready in $TC =="
