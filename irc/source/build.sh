#!/bin/bash
#
# build.sh <0.1.0|0.1.3> <freenode|lfnet>
#
# Cross-compiles the IRC-fixed Bitcoin GUI from source into
#   out/bitcoin-<version>-<network>.exe
#
# Run ./build-toolchain.sh once first (it builds wxWidgets 2.8, Berkeley DB 4.8,
# Boost and the libeay32 import lib into ./toolchain/).
#
# The IRC fix lives in bitcoin-<version>/irc.cpp:
#   * every IRC line the client sends ends in \r\n (modern ircd needs it; LFnet
#     tolerates it) instead of the original bare \r;
#   * the server host is the IRC_SERVER macro (default chat.freenode.net; the
#     lfnet build passes -DIRC_SERVER="irc.lfnet.org").
set -e
VER="${1:?usage: build.sh <0.1.0|0.1.3> <freenode|lfnet>}"
NET="${2:?usage: build.sh <0.1.0|0.1.3> <freenode|lfnet>}"
HOST=i686-w64-mingw32
HERE="$(cd "$(dirname "$0")" && pwd)"
TC="$HERE/toolchain"
SRCDIR="$HERE/bitcoin-$VER"
VENDOR="$HERE/../../bitcoin-nov08-rebuild/vendor"
WXCONF="$TC/wx/build/wx-config"
[ -d "$SRCDIR" ]        || { echo "no source for version $VER"; exit 1; }
[ -x "$WXCONF" ]        || { echo "toolchain missing — run ./build-toolchain.sh first"; exit 1; }

DEFS=(-DWIN32 -D__WXMSW__ -D_WINDOWS -DNOPCH)
case "$NET" in
  freenode)  DEFS+=(-DIRC_FORCE_XNICK) ;;             # default server; force x<digits> nick (Freenode Z-lines any base58-address nick)
  lfnet)     DEFS+=(-DIRC_SERVER='"irc.lfnet.org"') ;; # LFnet keeps the original 'u' nick (not banned there)
  *) echo "network must be freenode or lfnet"; exit 1 ;;
esac

INC=(-I"$SRCDIR" -I"$TC/src/boost_1_55_0" -I"$TC/db/build_unix" -I"$VENDOR" $("$WXCONF" --cxxflags))
CXXFLAGS=(-std=gnu++98 -fpermissive -w -mthreads -O2 -Wno-invalid-offsetof -fno-strict-aliasing "${DEFS[@]}" "${INC[@]}")

OBJ="$TC/obj-$VER-$NET"; mkdir -p "$OBJ" "$HERE/out"
for f in util script db net main market ui uibase sha irc; do
    echo "  CXX $f.cpp"
    $HOST-g++ -c "${CXXFLAGS[@]}" -o "$OBJ/$f.o" "$SRCDIR/$f.cpp"
done
echo "  RC  ui.rc"
RCFLAGS=()
for t in "${DEFS[@]}" "${INC[@]}"; do case "$t" in -I*|-D*) RCFLAGS+=("$t");; esac; done
$HOST-windres "${RCFLAGS[@]}" -o "$OBJ/ui_res.o" -i "$SRCDIR/ui.rc"

echo "  LINK out/bitcoin-$VER-$NET.exe"
LIBS=("$TC/db/build_unix/libdb_cxx-4.8.a" "$VENDOR/libeay32.a" $("$WXCONF" --libs std,richtext) -lws2_32 -lmswsock -lkernel32 -ladvapi32)
$HOST-g++ -mthreads -O2 -static -static-libgcc -static-libstdc++ \
    -o "$HERE/out/bitcoin-$VER-$NET.exe" "$OBJ"/*.o "${LIBS[@]}" -Wl,--subsystem,windows
$HOST-strip "$HERE/out/bitcoin-$VER-$NET.exe"
echo "DONE: out/bitcoin-$VER-$NET.exe  (needs libeay32.dll alongside at runtime)"
