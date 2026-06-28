# bitcoin-nov08-rebuild

A compilable reconstruction of Satoshi's November 2008 preview (`../bitcoin-nov08/`),
as a **wxWidgets GUI** node driven by a real window.

> **Status: builds and runs.** `bitcoin.exe` boots the nov08 core, creates the
> **byte-exact genesis** (`000006b1…`, merkle `769a5e93…`), brings up the P2P
> node on port **2222**, and mines from a button.
>
> The **consensus/genesis are byte-exact**. The window, however, is a *recreation*,
> not a recovery: the Nov-2008 preview never shipped its `ui.{h,cpp}`, so the UI is
> a small new front-end (`ui_app.cpp`), not Satoshi's original.

## Layout

Flat, like Satoshi's Nov-2008 preview — all source, the `makefile`, the prebuilt
`bitcoin.exe` and the DLLs it needs (`libeay32.dll`, `libgcc_s_dw2-1.dll`,
`libstdc++-6.dll`) live in this one folder (no `src/`). The period OpenSSL 0.9.8
headers come from the shared `../toolchain/openssl/`.

## What it is

The nov08 consensus + P2P core (Satoshi's `main`/`node` plus the reconstructed
support modules, a flat-file DB, and the `ui_app.cpp` wxWidgets front-end). It
keeps the **flat-file DB**, so it needs **wxWidgets only** — no Berkeley DB, no
Boost. `ui_app.cpp` runs the same boot sequence a console node would
(`LoadBlockIndex` → `LoadWallet`/`keyUser` → `LoadAddresses` → `StartNode`), opens
a `CMainFrame` status window, and wires up the core's existing `MainFrameRepaint()`
hook; a 1 Hz `wxTimer` repaints node state from the UI thread.

`bitcoin.exe` → `PE32 executable (GUI) Intel 80386`. wxWidgets is linked
**statically**; the C/C++ runtime is linked **dynamically** (the makefile, like
Satoshi's, has no `-static`), so it ships `libgcc_s_dw2-1.dll` + `libstdc++-6.dll`
alongside — the modern equivalent of the `mingwm10.dll` Satoshi shipped in 2009.
The committed binary is ready to run.

## How it is built

The `makefile` is **modelled on Satoshi's 0.1.0 `makefile`**, adapted to what the
nov08 preview is: `node.cpp` (not `net.cpp`) and the `ui_app.cpp` wxWidgets front
end (not `ui`/`uibase`/`irc`), a **flat-file DB** (no Berkeley DB / Boost), and two
necessary modern-GCC flags the 0.1.0 makefile didn't need — `-std=gnu++0x` (the
reconstructed `serialize.h` uses `std::true_type`) and `-fpermissive`.

Build with the shared toolchain (only wxWidgets is actually linked here):

```sh
../toolchain/build-toolchain.sh    # one time (builds wx 2.8 + BDB + Boost + libeay32.a)
../toolchain/make.sh               # drives the makefile -> bitcoin.exe
```

`make.sh` provides the cross-`g++` shim and points the makefile's
`INCLUDEPATHS`/`LIBPATHS`/`LIBS` at `../toolchain` — it does not edit the makefile.
Requirements: `g++-mingw-w64-i686`, `mingw-w64-tools`.

## Running it (under Wine)

```sh
WINEDEBUG=-all wine ./bitcoin.exe
```

The window shows the genesis hash, best block, height, peer count, wallet keys and
your address, with a **Start mining** button. Mining grows the chain at test
difficulty (`MINPROOFOFWORK=20`).

**Log / data location.** Unlike the 0.1.x clients (which use `%APPDATA%\Bitcoin`),
this build keeps its state **in the folder it runs from**: `blk0001.dat`,
`blkindex.dat`, `wallet.dat`, `addr.dat`. It is a **release** build, so it writes
**no `debug.log`**; its trace goes to `stdout`. `rm -f *.dat *.tmp` wipes the
state; the next run recreates the byte-exact genesis and a new key.

> It does **not** sync with today's Bitcoin network (2008 protocol, test
> difficulty). For studying/running the historical code, ideally in isolation.

## Reaching this version by patch

`patches/code/nov08-to-rebuild.patch` is the source diff from the Nov-2008 preview
to this folder (adds the support modules, flat-file DB and `ui_app.cpp`). To
regenerate this source from the preview:

```sh
cd bitcoin-nov08 && patch -p1 < ../patches/code/nov08-to-rebuild.patch
```

(It is mainly a reference diff — most of it is new files; see `patches/README.md`.)

## Honest limitations
- The UI is a **recreation**, not nov08's original (which doesn't survive); it is a
  minimal status window, not the full 0.1.0 client.
- The marketplace subsystem is an inert stub (unreleased and unrecoverable).
- The owner index and `SIGHASH_SINGLE/NONE` branches are approximated.

### Optional future directions

The GUI itself is **done** (this is a working wxWidgets node, not a plan). These are
*optional* extensions, not gaps:

- **IRC / peer discovery.** The Nov-2008 preview shipped **no** peer discovery (no
  IRC, no seeds), so this node only mines standalone. Grafting on the 0.1.0 IRC
  bootstrap (as in the `irc/` clients) would let it find peers — the most natural
  next step.
- **The fuller 0.1.0 UI.** This window is a minimal status view; back-porting
  0.1.0's `ui.cpp`/`uibase.cpp` (~3000 lines) would add the transaction list,
  address book and send dialogs, reconciling API drift (nov08's TxPos DB vs 0.1.0's
  `CTxDB`/`CTxIndex`, the unrecoverable market tab).
- **A native Linux build** — see the root `README.md` (partial POSIX shim + a wxGTK
  port).

A "purist" variant would also swap the flat-file store back to Berkeley DB 4.8 +
Boost (the toolchain already builds them, and
`patches/compilation/berkeley-db-4.8-gcc-atomic.patch` covers that fix).
