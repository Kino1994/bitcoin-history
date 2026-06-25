# nov08-rebuild - Makefile
#
# Primary (chosen) target: an AUTHENTIC Windows XP build with MinGW, linking the
# libeay32.dll (OpenSSL 0.9.8) that Satoshi shipped together with the period
# OpenSSL 0.9.8 headers vendored in ../vendor. That way bignum.h and key.h
# compile WITHOUT modification.
#
#   make implib   -> generate ../vendor/libeay32.a from the DLL (one time only)
#   make xp       -> cross-compile bitcoin.exe for Windows XP
#   make clean
#
# Requirements (install with sudo):
#   apt-get install g++-mingw-w64-i686 mingw-w64-tools
#
# (A native Linux build is left as 'make native', but it clashes with OpenSSL
#  3.0; the supported path is XP/MinGW.)

SRCS = sha.cpp util.cpp script.cpp db.cpp market.cpp main.cpp node.cpp init.cpp

VENDOR = ../vendor

# --- Windows XP cross-build (MinGW i686, OpenSSL 0.9.8) ---
XPCXX   = i686-w64-mingw32-g++
XPFLAGS = -std=c++11 -w -fpermissive -I. -I$(VENDOR) -march=i686 -mconsole -DWIN32 -D_WIN32_WINNT=0x0400 \
          -static -static-libgcc -static-libstdc++
XPLIBS  = $(VENDOR)/libeay32.a -lws2_32 -lgdi32

xp: $(VENDOR)/libeay32.a
	$(XPCXX) $(XPFLAGS) -o bitcoin.exe $(SRCS) $(XPLIBS)
	@echo "OK -> bitcoin.exe  (place next to libeay32.dll on Windows XP)"

# Import library from the DLL Satoshi shipped (OpenSSL 0.9.8)
$(VENDOR)/libeay32.a: $(VENDOR)/libeay32.dll
	cd $(VENDOR) && gendef libeay32.dll
	i686-w64-mingw32-dlltool -d $(VENDOR)/libeay32.def -l $(VENDOR)/libeay32.a -D libeay32.dll
	@echo "OK -> import lib generated"

implib: $(VENDOR)/libeay32.a

# Run the node. In this Linux dev environment it launches under Wine; on a real
# Windows XP machine you just run bitcoin.exe directly. Use `make run ARGS=-gen`
# to mine. Ctrl-C to stop.
run:
	@test -f bitcoin.exe || $(MAKE) xp
	@cp -f $(VENDOR)/libeay32.dll .
	WINEDEBUG=-all wine ./bitcoin.exe $(ARGS)

# Wipe all node data (blocks, tx index, wallet, peer addresses). The next run
# recreates the genesis block and a brand-new key. WARNING: discards the wallet.
reset:
	rm -f *.dat *.tmp
	@echo "node data wiped (next run recreates the genesis block and a new key)"

# --- Native Linux build (verification; needs a modernized bignum.h, not used) ---
native: $(SRCS)
	g++ -std=c++11 -w -fpermissive -I. -c $(SRCS)

clean:
	rm -f *.o bitcoin.exe $(VENDOR)/libeay32.def $(VENDOR)/libeay32.a *.tmp

.PHONY: xp implib run reset native clean
