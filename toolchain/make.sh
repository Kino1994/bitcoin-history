#!/bin/bash
#
# make.sh — build a Bitcoin 0.1.x GUI from its ORIGINAL Satoshi `makefile`,
# unchanged, using the shared period toolchain. Run it from inside the source
# folder (the one that contains Satoshi's `makefile`):
#
#   cd bitcoin-0.1.0/src && ../../toolchain/make.sh
#   cd bitcoin-0.1.3/src && ../../toolchain/make.sh
#
# It does NOT edit the makefile or the source. It only adapts the *environment*:
#   * a PATH shim so the makefile's literal `g++`/`windres` resolve to the
#     i686-w64-mingw32 cross tools, with `g++` driven in the period C++ standard
#     (-std=gnu++98) so 2009 code (e.g. an unqualified `array`) still compiles
#     under a modern GCC;
#   * overrides of the makefile's own INCLUDEPATHS / LIBPATHS / LIBS variables
#     (and BUILD=release) so they point at ../toolchain instead of the 2009
#     Windows install paths (/boost, /DB, /OpenSSL, /wxWidgets).
#
# After building it PACKAGES the result at the deliverable root: bitcoin.exe is
# stripped and placed there together with the two MinGW runtime DLLs it needs
# (libgcc_s_dw2-1.dll, libstdc++-6.dll). "Deliverable root" is the folder itself
# for the flat projects (irc/<net>/bitcoin-*, bitcoin-nov08-rebuild) and the
# PARENT for the 0.1.x releases (which build in src/, so the exe lands next to the
# folder's original bitcoin.exe — replacing it in the working tree only; the
# committed exe there stays Satoshi's original, see the repo's .gitignore /
# skip-worktree note).
#
# Extra args are passed through to make (e.g. `make.sh clean`).
set -e
TC="$(cd "$(dirname "$0")" && pwd)"
WXCFG="$TC/build/wx/build/wx-config"
[ -x "$WXCFG" ] || { echo "toolchain missing — run $TC/build-toolchain.sh first"; exit 1; }
[ -f makefile ] || { echo "run this from a folder containing Satoshi's 'makefile'"; exit 1; }

SHIM="$(mktemp -d)"
trap 'rm -rf "$SHIM"' EXIT
printf '#!/bin/sh\nexec i686-w64-mingw32-g++ -std=gnu++98 "$@"\n'  > "$SHIM/g++";     chmod +x "$SHIM/g++"
printf '#!/bin/sh\nexec i686-w64-mingw32-windres "$@"\n'          > "$SHIM/windres"; chmod +x "$SHIM/windres"

# only -I/-D from wx-config (the makefile feeds INCLUDEPATHS to windres, which
# rejects compiler flags like -mthreads)
WXID=$("$WXCFG" --cxxflags | tr ' ' '\n' | grep -E '^-I|^-D' | tr '\n' ' ')
INC="$WXID -I$TC -I$TC/build/src/boost_1_55_0 -I$TC/build/db/build_unix"
LP="-L$TC/build/db/build_unix -L$TC/build"
LIBS="-ldb_cxx -leay32 $("$WXCFG" --libs std,richtext) -lws2_32 -lmswsock"

mkdir -p obj
PATH="$SHIM:$PATH" make BUILD=release INCLUDEPATHS="$INC" LIBPATHS="$LP" LIBS="$LIBS" "$@"

# --- package the result at the deliverable root (only on a normal build) ---
if [ -f bitcoin.exe ]; then
    DEST=.
    [ "$(basename "$PWD")" = src ] && DEST=..   # 0.1.x: exe belongs at the folder root
    DLLDIR="$(dirname "$(i686-w64-mingw32-g++ -print-libgcc-file-name)")"
    i686-w64-mingw32-strip bitcoin.exe
    for dll in libgcc_s_dw2-1.dll libstdc++-6.dll; do
        if [ -f "$DLLDIR/$dll" ]; then
            cp -f "$DLLDIR/$dll" "$DEST/$dll"
            i686-w64-mingw32-strip "$DEST/$dll" 2>/dev/null || true
        fi
    done
    if [ "$DEST" != "." ]; then mv -f bitcoin.exe "$DEST/bitcoin.exe"; fi
    rm -rf obj headers.h.gch
    echo "OK -> $DEST/bitcoin.exe  (+ runtime DLLs)"
fi
