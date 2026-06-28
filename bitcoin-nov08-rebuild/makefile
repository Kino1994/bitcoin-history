# nov08-rebuild makefile — modelled on Satoshi's 0.1.0 makefile, adapted to the
# November 2008 preview. Differences from 0.1.0 (all because of what nov08 is):
#   * source set: node.cpp (not net.cpp) and the ui_app.cpp wxWidgets front-end
#     (not ui.cpp/uibase.cpp/irc.cpp), no ui.rc resource, no market UI;
#   * a flat-file DB (the reconstructed db.cpp), so NO Berkeley DB (-ldb_cxx);
#   * -std=gnu++0x: the reconstructed serialize.h uses std::true_type, a C++11
#     feature 0.1.0 didn't need (0.1.0 builds in the compiler default / gnu++98);
#   * -fpermissive: the reconstructed main.cpp takes the address of an rvalue in
#     one spot, which a modern GCC rejects without it (0.1.0 didn't need it).
#
# Build with the shared toolchain:  ../toolchain/make.sh   (it provides the cross
# g++ shim and points INCLUDEPATHS/LIBPATHS/LIBS at ../toolchain; the -std=gnu++0x
# in CFLAGS below overrides the shim's period default).

ifneq "$(BUILD)" "debug"
ifneq "$(BUILD)" "release"
BUILD=debug
endif
endif
ifeq "$(BUILD)" "debug"
D=d
DEBUGFLAGS=-g -D__WXDEBUG__
endif

INCLUDEPATHS=-I"/OpenSSL/include" -I"/wxWidgets/lib/vc_lib/mswd" -I"/wxWidgets/include"
LIBPATHS=-L"/OpenSSL/out" -L"/wxWidgets/lib/gcc_lib"
LIBS= \
 -l eay32 \
 -l wxmsw28$(D)_richtext -l wxmsw28$(D)_html -l wxmsw28$(D)_core -l wxbase28$(D) -l wxtiff$(D) -l wxjpeg$(D) -l wxpng$(D) -l wxzlib$(D) -l wxregex$(D) -l wxexpat$(D) \
 -l kernel32 -l user32 -l gdi32 -l comdlg32 -l winspool -l winmm -l shell32 -l comctl32 -l ole32 -l oleaut32 -l uuid -l rpcrt4 -l advapi32 -l ws2_32
WXDEFS=-DWIN32 -D__WXMSW__ -D_WINDOWS -DNOPCH -D_WIN32_WINNT=0x0400
CFLAGS=-mthreads -O0 -w -Wno-invalid-offsetof -Wformat -std=gnu++0x -fpermissive $(DEBUGFLAGS) $(WXDEFS) -I. $(INCLUDEPATHS)
HEADERS=headers.h compat.h serialize.h uint256.h util.h key.h bignum.h base58.h script.h db.h market.h node.h main.h


all: bitcoin.exe


headers.h.gch: headers.h $(HEADERS)
	g++ -c $(CFLAGS) -o $@ $<

obj/sha.o: sha.cpp		sha.h
	g++ -c $(CFLAGS) -O3 -o $@ $<

obj/util.o: util.cpp		$(HEADERS)
	g++ -c $(CFLAGS) -o $@ $<

obj/script.o: script.cpp	$(HEADERS)
	g++ -c $(CFLAGS) -o $@ $<

obj/db.o: db.cpp		$(HEADERS)
	g++ -c $(CFLAGS) -o $@ $<

obj/market.o: market.cpp	$(HEADERS) market.h
	g++ -c $(CFLAGS) -o $@ $<

obj/main.o: main.cpp		$(HEADERS) node.h sha.h
	g++ -c $(CFLAGS) -o $@ $<

obj/node.o: node.cpp		$(HEADERS) node.h
	g++ -c $(CFLAGS) -o $@ $<

obj/ui_app.o: ui_app.cpp	$(HEADERS)
	g++ -c $(CFLAGS) -o $@ $<


OBJS=obj/sha.o obj/util.o obj/script.o obj/db.o obj/market.o obj/main.o obj/node.o obj/ui_app.o

bitcoin.exe: headers.h.gch $(OBJS)
	g++ $(CFLAGS) -mwindows -Wl,--subsystem,windows -o $@ $(LIBPATHS) $(OBJS) $(LIBS)

clean:
	-del /Q obj\*
	-del /Q headers.h.gch bitcoin.exe
