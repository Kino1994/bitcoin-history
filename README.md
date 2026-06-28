# bitcoin-history

Early Bitcoin source archives.

The archived source code is taken from the Satoshi Nakamoto Institute:
<https://satoshi.nakamotoinstitute.org/code/>

- **bitcoin-nov08/** — Satoshi's November 2008 pre-release source preview (only `main` + `node`; doesn't compile on its own).
- **bitcoin-nov08-rebuild/** — a compilable reconstruction of that preview as a **wxWidgets GUI** node (`ui_app.cpp`). The **consensus/genesis are byte-exact**; the UI is a recreation, not a recovery (nov08's own `ui.cpp` never shipped). Builds and runs (mines from a button); includes a prebuilt `bitcoin.exe`. See its README.
- **bitcoin-0.1.0/** — Satoshi's first public release, v0.1.0 ALPHA (January 2009): original binary + source (`src/`, buildable with the shared toolchain).
- **bitcoin-0.1.3/** — Bitcoin v0.1.3 release: original binary + source.
- **irc/** — the 0.1.0 and 0.1.3 GUI clients **rebuilt from source with an IRC fix** so they can still bootstrap peers in 2026. `irc/freenode/` and `irc/lfnet/` each hold `bitcoin-0.1.0/` and `bitcoin-0.1.3/` — self-contained flat folders that are exactly `(bitcoin-0.1.x/src + the network's patch)`, **Satoshi's own `makefile`** included, plus the ready-to-run `bitcoin.exe` (network baked into `irc.cpp`). See "Reaching peers via IRC" below.
- **toolchain/** — the shared period build stack used by every from-source build (the nov08 rebuild, 0.1.0/0.1.3, the IRC clients). `build-toolchain.sh` cross-builds wxWidgets 2.8.12 + Berkeley DB 4.8 + Boost into `toolchain/build/` (git-ignored); `toolchain/openssl/` holds the period OpenSSL 0.9.8 headers.
- **patches/** — version-to-version source diffs (`code/`: nov08 → rebuild, nov08 → 0.1.0, 0.1.0 → 0.1.3, and the IRC fixes per network) and modern-toolchain library fixes (`compilation/`: wxWidgets 2.8.12, Berkeley DB 4.8). See `patches/README.md`.

**Everything except `bitcoin-nov08/` builds from source** (it is just Satoshi's preview and does not compile). The originals' binaries are kept as committed; the rebuilds/IRC clients ship a freshly-built `bitcoin.exe`.

## Running each version

Every folder holds a `bitcoin.exe` (always that name) plus the `libeay32.dll` it
needs alongside. On Windows XP just double-click it; on Linux run it under **Wine**
(`WINEDEBUG=-all wine ./bitcoin.exe` to silence noise). `nov08/` is the only
folder with no exe — it is Satoshi's source preview and does not compile on its own.

```sh
# Satoshi's original GUI releases
cd bitcoin-0.1.0 && wine ./bitcoin.exe                 # GUI (Jan 2009)
cd bitcoin-0.1.3 && wine ./bitcoin.exe                 # GUI

# nov08 preview, reconstructed as a wxWidgets GUI (byte-exact consensus core)
cd bitcoin-nov08-rebuild && wine ./bitcoin.exe         # GUI (Start-mining button)

# IRC-patched 0.1.x GUI clients that still find peers in 2026 — one exe per network:
cd irc/freenode/bitcoin-0.1.0 && wine ./bitcoin.exe    # today's Freenode (registers, mines)
cd irc/freenode/bitcoin-0.1.3 && wine ./bitcoin.exe
cd irc/lfnet/bitcoin-0.1.0    && wine ./bitcoin.exe    # LFnet (discovers live peers)
cd irc/lfnet/bitcoin-0.1.3    && wine ./bitcoin.exe
```

> The 0.1.x GUI clients (stock and IRC) and the nov08 rebuild can look frozen /
> "Not Responding" for a minute or two at startup — let them wait, don't force-close.

### Rebuilding from source

Build the shared toolchain once, then `make` in any project. Everything except
`bitcoin-nov08/` builds from source.

```sh
toolchain/build-toolchain.sh     # one time: wx 2.8 + Berkeley DB 4.8 + Boost + libeay32.a (~15 min)
```

| Target | How (after the toolchain is built) |
|--------|-----|
| `bitcoin-nov08-rebuild` | `cd` there, `../toolchain/make.sh` — a `makefile` modelled on Satoshi's 0.1.0 (adapted: flat-file DB so only wxWidgets is linked; `-std=gnu++0x` + `-fpermissive` for the reconstructed code) |
| `bitcoin-0.1.0` / `bitcoin-0.1.3` | `cd src && ../../toolchain/make.sh` — builds from **Satoshi's own unmodified `makefile`**. The result is placed at the **folder root, over the original `bitcoin.exe`** (so a rebuild replaces it in place), but the committed exe stays Satoshi's original — see below |
| IRC clients (all 4) | `cd` into any `irc/<freenode\|lfnet>/bitcoin-<0.1.0\|0.1.3>/` and run `../../../toolchain/make.sh` — also Satoshi's unmodified `makefile`; the network is baked into the patched `irc.cpp` |

The IRC clients can equivalently be reached as **source** by applying the
per-network code patches — see `patches/README.md`.

#### Applying the patches

Two kinds live in `patches/` (full details there):

- **`patches/code/`** — version-to-version *source* diffs. Apply with `-p1` onto a
  copy of the "from" tree, e.g.:
  ```sh
  cd bitcoin-0.1.0/src && patch -p1 < ../../patches/code/lfnet/0.1.0.patch   # -> LFnet client source
  cd bitcoin-nov08    && patch -p1 < ../patches/code/nov08-to-rebuild.patch  # -> the GUI rebuild source
  ```
- **`patches/compilation/`** — modern-toolchain fixes for the *libraries*, applied
  inside each dependency's own tree (`toolchain/build-toolchain.sh` does this
  automatically):
  ```sh
  patch -p1 < patches/compilation/wxWidgets-2.8.12-mingw.patch       # in wxWidgets-2.8.12/
  patch -p1 < patches/compilation/berkeley-db-4.8-gcc-atomic.patch   # in db-4.8.30.NC/
  ```

#### Logs and data, per build

| Binary | `debug.log`? | trace | wallet/chain/peers |
|--------|--------------|-------|--------------------|
| Satoshi's original 0.1.0/0.1.3 (debug builds) | **yes** (+ `db.log`), in **the run folder** | — | `%APPDATA%\Bitcoin` |
| 0.1.x rebuilt from source, and the `irc/` clients (release) | **no** | `stdout` (block-buffered under Wine) | `%APPDATA%\Bitcoin` |
| `bitcoin-nov08-rebuild` (release) | **no** | `stdout` | **the run folder** (`blk0001.dat`, `wallet.dat`, …) |

Mind the split for the 0.1.0/0.1.3 builds: **logs and data go to different
places.** The two logs are opened with a *relative* path, so they land in the
**current working directory** — the folder you `cd`'d into to launch — not in the
data dir: `debug.log` (`util.h`, `fopen("debug.log", "a")`) and `db.log`
(Berkeley DB's error file, `db.cpp`, `dbenv.set_errfile(fopen("db.log", ...))`).
The wallet, chain and peer **data** go to `GetAppDir()` = `%APPDATA%\Bitcoin`
(`main.cpp`), which under Wine is
`~/.wine/drive_c/users/$USER/Application Data/Bitcoin/`. That folder holds
`wallet.dat`, `blkindex.dat`, `blk0001.dat`, `addr.dat`, **and a `database/`
subfolder** — the Berkeley DB environment (`db.cpp` sets `set_lg_dir(AppDir +
"\\database")`), whose `log.000000000N` files are recreated on every run.

**Preview vs release — same database in Satoshi's design, different layer in this
rebuild.** Satoshi used **Berkeley DB throughout**: the November 2008 preview's
own source already calls `CTxDB`/`CWalletDB` (the BDB-backed classes over `CDB`),
exactly as 0.1.0/0.1.3 do. But the leaked nov08 archive shipped **without
`db.cpp`/`db.h`**, so `bitcoin-nov08-rebuild` reconstructs that missing layer as
**flat files** — plain relative `fopen` (`blk0001.dat`, `wallet.dat`, …) that land
in the **run folder**, with **no `database/` environment at all** (hence no
`DB_RUNRECOVERY` failure mode). That flat-file store is a rebuild simplification,
not Satoshi's choice. The real **Berkeley DB 4.8** environment under
`%APPDATA%\Bitcoin\database\` therefore exists only in the 0.1.0/0.1.3 **release**
builds (stock and the `irc/` rebuilds) — the only build family where the recovery
note below applies.
The release-vs-debug reason is explained under "Building the GUI from source" below.

#### Building 0.1.0 / 0.1.3 from source (Satoshi's own makefile)

`bitcoin-0.1.0/src` and `bitcoin-0.1.3/src` are kept **exactly as Satoshi shipped
them**, his `makefile` included. To rebuild from that original makefile:

```sh
toolchain/build-toolchain.sh          # one time
cd bitcoin-0.1.0/src && ../../toolchain/make.sh
```

`toolchain/make.sh` does **not** edit the makefile or the source. It only adapts
the *environment* so a 2009 makefile drives a 2026 cross-compiler:

- a PATH shim so the makefile's literal `g++`/`windres` resolve to
  `i686-w64-mingw32-*`, with `g++` forced into the period **`-std=gnu++98`** (2009
  code uses an unqualified `array`, which clashes with C++11's `std::array`);
- overrides of the makefile's own `INCLUDEPATHS` / `LIBPATHS` / `LIBS` (and
  `BUILD=release`) to point at `toolchain/` instead of the 2009 Windows install
  paths (`/boost`, `/DB`, `/OpenSSL`, `/wxWidgets`).

`make.sh` then places the result at the **folder root** — the stripped `bitcoin.exe`
over the existing one, plus the two MinGW runtime DLLs it needs
(`libgcc_s_dw2-1.dll`, `libstdc++-6.dll`) — so a rebuild **replaces the exe in
place**, ready to run. This is for anyone who wants to regenerate the binaries from
this repo.

**But the committed exe stays Satoshi's ORIGINAL.** In this working copy it is kept
canonical with `git update-index --skip-worktree bitcoin-0.1.x/bitcoin.exe`, so a
local rebuild never dirties or commits it (and the rebuild's runtime DLLs at the
root are git-ignored).

Note that `skip-worktree` is a **per-clone** git flag — it is *not* stored in the
repo and does not travel on clone. A fresh clone therefore gets the committed
original, and (lacking the flag) a rebuild simply shows up as an ordinary
modification — so whoever clones can just **commit the regenerated exe** if they
want to rewrite it, or `git checkout -- bitcoin-0.1.0/bitcoin.exe` to restore the
original. To get the same don't-dirty protection locally, set the flag yourself;
to deliberately replace it here, clear it once with
`git update-index --no-skip-worktree bitcoin-0.1.0/bitcoin.exe`.

#### How this toolchain differs from Satoshi's (2009)

The **source and the makefiles are Satoshi's, unchanged** — every difference is in
the *toolchain around them*, forced by building with a 2026 compiler instead of a
2009 one. The full list:

| | Satoshi, 2009 | here, 2026 | why / consequence |
|---|---|---|---|
| **Build host** | native **Windows** (MinGW shell) | **Linux**, cross-compiling (also buildable on Windows) | convenience; the makefile supports both |
| **Compiler** | MinGW **GCC ~3.4** | **MinGW-w64 GCC 10** | the root cause of everything below |
| **C++ standard** | the era default (≈C++98) | forced **`-std=gnu++98`** (0.1.x/IRC); **`-std=gnu++0x` + `-fpermissive`** (nov08 rebuild) | GCC 10 defaults to gnu++14: an unqualified `array` clashes with `std::array`; the reconstructed code also uses `std::true_type` (C++11) and takes the address of an rvalue |
| **Library source fixes** | none | **2 patches** (`patches/compilation/`) | GCC 10 rejects Berkeley DB's `__atomic_compare_exchange` (now a builtin) and wxWidgets' `_mkdir`/`_rmdir` (need `<direct.h>` on 32-bit MinGW-w64); a `case -1` over a `DWORD` is sidestepped by gnu++98, no flag |
| **Resource compiler** | plain `windres` | `windres` fed **only `-I`/`-D`** | `wx-config --cxxflags` now emits flags (`-mthreads`, …) that `windres` rejects |
| **C/C++ runtime DLLs** | **`mingwm10.dll`** (in his exe's imports) | **`libgcc_s_dw2-1.dll` + `libstdc++-6.dll`** | the makefile links the runtime dynamically (no `-static`); these are the modern equivalents, shipped next to the exe |
| **OpenSSL import lib** | linked his `.lib` directly | **`libeay32.a` generated** from his `libeay32.dll` via `gendef`/`dlltool` | cross-tools need an import lib; the DLL itself is **his, unchanged** |
| **wxWidgets** | 2.8.x (ANSI, static) | **2.8.12** (ANSI, static) — pinned | — |
| **Berkeley DB** | 4.x | **4.8.30.NC** | — |
| **Boost** | period (~1.3x, headers only) | **1.55** (headers only) | Bitcoin links no Boost libs, so the version is flexible |
| **OpenSSL** | **0.9.8** + his `libeay32.dll` | **the same** — period 0.9.8 headers + his own `libeay32.dll` | identical crypto |
| **Build mode** | his distributed binaries were **debug** (they write `debug.log`) | **release** (`BUILD=release`, no `debug.log`) | must match the release wxWidgets libs (see "Release vs. debug" below) |
| **Packaging** | shipped as-is | `bitcoin.exe` + runtime DLLs **stripped** (~6.4 MB, his original's size) | a packaging step; machine code unchanged |

**Net result:** because the compiler differs, a rebuilt exe is **not byte-identical**
to Satoshi's — which is exactly why his originals (0.1.0/0.1.3) are kept committed
and a rebuild only replaces them in the working tree. The detailed write-ups of the
build errors, the release/debug split and the IP-echo startup fix are in the
sections below.

#### Building on Windows (Satoshi's native path)

The `makefile` is the **original Windows build file** — bare `g++`/`windres` (off
the `PATH`), its `clean` target uses `del`, and its default `INCLUDEPATHS` /
`LIBPATHS` point at C:-drive install roots (`\boost`, `\DB\build_unix`,
`\OpenSSL`, `\wxWidgets`). So the executables can be produced **natively on
Windows**, exactly as Satoshi built them — no Linux or Wine involved:

```bat
:: in a MinGW shell, with g++/windres on PATH and the period deps installed
:: at the makefile's expected roots (or pass the same INCLUDEPATHS/LIBPATHS/LIBS
:: overrides toolchain/make.sh uses):
cd bitcoin-0.1.0\src
make                  :: BUILD=debug — what Satoshi shipped (writes debug.log)
make BUILD=release    :: release build
```

This works the same for **0.1.0/0.1.3**, the **IRC clients** (`irc/<net>/bitcoin-*`
— same makefile, network baked into the source), and **`bitcoin-nov08-rebuild`**
(its makefile is modelled on the same one). Two cases:

- **Period MinGW (GCC ~3.4, as in 2009):** builds clean, *no* patches/flags — it
  is the 2009 build verbatim.
- **A modern MinGW (GCC 10+) on Windows:** hits the **same** issues we bridge when
  cross-building, so apply the same `patches/compilation/` fixes and the
  `-std=gnu++98` (`-std=gnu++0x` for the nov08 rebuild) standard.

In short: **Linux** (cross, via `toolchain/make.sh`) and **Windows** (native `make`)
both produce the Windows `.exe` from the unchanged source + makefile.

#### A native *Linux* binary (ELF) — limitations

This is a different target, and it is **not** provided here. Producing a native
Linux ELF (rather than a Windows `.exe` run under Wine) runs into four things:

- **The Win32→POSIX shim is partial.** The nov08 rebuild ships `compat.h` and a
  `winsock2.h` shim (Winsock→BSD sockets) "for native verification", and pure
  files do compile to ELF (`sha.cpp` builds cleanly). But the shim is incomplete:
  compiling `util.cpp` natively still fails on un-shimmed Win32 APIs —
  thread sync (`CreateMutex` / `WaitForSingleObject` / `ReleaseMutex` /
  `CloseHandle`, `INFINITE`) and registry-based entropy (`HKEY_PERFORMANCE_DATA`,
  which `RandAddSeed` reads). A native build must map these to POSIX
  (pthread mutexes, `/dev/urandom`), plus other touch-points (`ShellExecute`, the
  single-instance mutex, the `%APPDATA%` data path).
- **OpenSSL version.** The code targets the **0.9.8** API (`bignum.h`/`key.h` use
  structs and calls that became opaque or changed in 1.1/3.0); a typical Linux
  system ships **OpenSSL 3.0+**. Cross-compiling sidesteps this by linking
  Satoshi's own `libeay32.dll` + the period headers; a native link would need a
  period OpenSSL 0.9.8 (or API-compat shims).
- **The GUI is the MSW port.** wxWidgets here is built and used as `__WXMSW__`
  (`ui.cpp`/`uibase.cpp` and even `ui_app.cpp`, the `ui.rc` resources). A native
  Linux GUI needs a **wxGTK** build and porting the windowing/resource code.
- **No byte-fidelity anyway.** Like the Windows rebuilds, a native binary would be
  a different artifact from Satoshi's 2009 exe.

So scope-wise: a native Linux **console** node is a *bounded* project (finish the
POSIX shim + use a period OpenSSL); a native Linux **GUI** is a *larger* port
(wxGTK). Neither is done here — on Linux the supported path is the Windows `.exe`
under **Wine**.

Notes on the 0.1.0 / 0.1.3 GUI clients:

- **The stock clients may look frozen / "Not Responding" for a minute or two at
  startup — let them wait, don't force them to close.** Before bringing up the
  network, the client calls `GetMyExternalIP`, which dials **hard-coded IPs** the
  IP-echo services used back in 2008 (`72.233.89.199` for *whatismyip.com* in
  0.1.0; `70.86.96.218` and `208.78.68.70` in 0.1.3). Those sites are still alive
  but moved off those addresses years ago (they now sit behind Cloudflare/Oracle),
  so the baked-in IPs no longer answer and the connect blocks until it times out —
  twice, in 0.1.3. If Wine — or Windows, running the `.exe` natively — pops a "the
  program is not responding" dialog, choose **Wait** (you may have to a few times);
  do **not** click *End/Close process*. It recovers on its own and runs normally.
  *(The `irc/` builds fix this: they resolve those services by **domain name**
  instead of the dead IPs, so they reach the network in seconds — see
  `GetMyExternalIP` in `irc/*/bitcoin-*/net.cpp`.)*
- They enforce a **single instance** (shared between both). If one is already
  open, or a previous run was killed instead of closed, the next launch quits
  silently ("Existing instance found"). Run `wineserver -k` before relaunching.
- They are **windowed apps** (need a display) and store their data in
  `%APPDATA%\Bitcoin` — under Wine that's
  `~/.wine/drive_c/users/<user>/Application Data/Bitcoin/`
  (`wallet.dat`, `blkindex.dat`, `blk0001.dat`, `addr.dat`, plus a `database/`
  Berkeley DB environment), **not** the exe folder — the `debug.log`/`db.log`
  traces are the exception, they go to whatever folder you launched from. They
  connect to the real network on port 8333.
- **If a launch dies instantly with `DB_RUNRECOVERY` — `DbRunRecoveryException`
  in `OnInit()`, and `db.log` shows `unsupported log version NN` /
  `PANIC: Invalid argument` — the Berkeley DB environment is corrupt; wipe it,
  there is no in-place recovery for this old code.** These debug builds link
  **Berkeley DB 4.8**, and the environment lives in
  `%APPDATA%\Bitcoin\database\` (`log.000000000N`). If a newer BDB ever touched
  that Wine prefix, or a run was killed mid-write, its log format is no longer
  one BDB 4.8 understands and every subsequent start aborts. Fix it by deleting
  the environment and letting the client rebuild it:
  `rm -rf "$HOME/.wine/drive_c/users/$USER/Application Data/Bitcoin/database"`
  (or clear the whole `Bitcoin/` folder to also drop `wallet.dat` and the chain
  and restart from the January 2009 genesis). Everything is regenerated on the
  next launch.
- Out of the box they stay at **height 0** (the real genesis, carrying the
  *Times* headline) for two reasons. First, peer discovery (IRC, via Freenode)
  still *connects* but no longer completes registration ("Registration timeout"),
  so they find no peers and download nothing — though this *can* be bridged (see
  "Reaching peers via IRC" below). Second, the miner deliberately refuses to run
  while isolated — it blocks on `while (vNodes.empty()) Sleep(1000)` — so enabling
  *Options → Generate Coins* prints `BitcoinMiner started` but then just sleeps
  (CPU ~0%, no blocks). The chain never grows until it has at least one peer.

By contrast, **bitcoin-nov08-rebuild** is even more isolated: the November 2008
preview has *no peer discovery at all* — no IRC, no seeds, nothing but an (empty)
`addr.dat` — so it can never find anyone (Satoshi added the IRC bootstrap only
for the public 0.1.0 release). But the nov08 miner also lacks the
`while (vNodes.empty())` check, so the rebuild's **Start mining** button mines
standalone at a trivial test difficulty and `best_height` climbs anyway. So the reason the 0.1.x
clients can't grow their chain isn't the difficulty — it's that their miner won't
run without a connection, a safeguard the older preview didn't have yet.

This was verified experimentally. A node adds an inbound socket to `vNodes` the
moment it `accept()`s it, before any handshake (`net.cpp`), so simply holding a
TCP connection open to its port 8333 is enough to make `vNodes` non-empty. With
*Generate Coins* on and that one fake peer, both clients left the
`while (vNodes.empty())` loop, started hashing (CPU ~100%, `Running BitcoinMiner`)
and mined real blocks on top of the January 2009 genesis at difficulty 1, both
reaching height 2. At difficulty 1 with this single-threaded
2009 code under Wine a block takes roughly 15–20 min. Drop the connection and the
miner goes back to sleep. This confirms the gate is peer connectivity, not
proof-of-work difficulty.

So the 0.1.x clients **can** still grow their height — you just have to supply the
peer yourself, because IRC is their only auto-discovery and neither 0.1.0 nor
0.1.3 has a `-connect`/`-addnode` option (they're identical here). Give them at
least one peer — two 0.1.x nodes on the same LAN (peered via a pre-seeded
`addr.dat`), the held-socket trick above, or the IRC route below — and with one
of them mining the height climbs, building a **private fork on top of the
original January 2009 blockchain** that the public network would never accept.

### Reaching peers via IRC

Out of the box both clients are hard-coded to `chat.freenode.net` (`#bitcoin`,
port 6667 — see `irc.cpp`) with no flag to change it. The discovery itself is
simple: register on IRC, `JOIN #bitcoin`, `WHO`, and decode the channel's
`u<base58check>` nicks — each encodes a peer's IP:port via a packed
`struct ircaddr { int ip; short port; }` (`EncodeAddress` in `irc.cpp`) — back
into addresses and connect to them. The question is whether the IRC servers of
2026 still let a 2009 client through. We tried both networks and made each work.

#### The `irc/` folder — 0.1.0 / 0.1.3 rebuilt from source with the IRC fix

The `irc/` folder holds the 0.1.0 and 0.1.3 GUI clients **recompiled from
source** with a one-line-class fix in `irc.cpp`, so each talks to a live IRC
network again:

```
irc/
  build-toolchain.sh                          # builds the shared period toolchain
  freenode/
    bitcoin-0.1.0/  bitcoin-0.1.3/            # flat source + Makefile + bitcoin.exe
  lfnet/
    bitcoin-0.1.0/  bitcoin-0.1.3/            # flat source + Makefile + bitcoin.exe
```

Each client folder is **self-contained and flat** (like the root `bitcoin-0.1.0/`):
it is exactly the result of applying `patches/code/<network>/<version>.patch` to
the matching `bitcoin-0.1.x/src` — **Satoshi's own `makefile` included, unchanged**
— plus the prebuilt `bitcoin.exe` and the DLLs it needs alongside. The IRC network
is **baked into `irc.cpp`**, so it builds with no flags via
`../../../toolchain/make.sh` (which drives that original makefile). Run any one
directly (GUI): `cd irc/lfnet/bitcoin-0.1.0 && wine ./bitcoin.exe`.

**The fix (`irc/<network>/bitcoin-*/irc.cpp`).** Three small changes:
- Every IRC line the client *sends* now ends in `\r\n` instead of the original
  bare `\r` — the four `Send()` format strings (`NICK`, `USER`, `JOIN`, `WHO`)
  and the `strLine += '\r'` that builds the `PONG`. Modern Freenode (InspIRCd-3)
  requires `\r\n` (it does accept a bare `\n`, but `\r\n` is the RFC-correct form
  and LFnet tolerates it too), so this single change makes the client register on
  *both* networks.
- The hard-coded server became an `IRC_SERVER` macro (default
  `chat.freenode.net`); the LFnet folders bake `#define IRC_SERVER "irc.lfnet.org"`
  into `irc.cpp`.
- The **Freenode** folders bake `#define IRC_FORCE_XNICK`, which forces the
  `x<digits>` nick. This is the subtle one. Once the `GetMyExternalIP` fix gives
  the node its real (routable) external IP, it would advertise itself with a nick
  that base58-encodes that address — and Freenode's anti-spam **Z-lines** any
  base58-address nick (layer 4 below; we watched it register fully, then get
  Z-lined seconds later). Forcing `x<digits>` (which carries no address, so the
  filter ignores it) lets the client register and stay; you can't advertise a peer
  on Freenode anyway. On LFnet the address nick is kept, because LFnet doesn't ban
  it and it advertises a real connectable address.

Those are the only per-network differences (`IRC_SERVER` for LFnet,
`IRC_FORCE_XNICK` for Freenode), and they are the one line that the freenode and
lfnet code patches differ by.

**Rebuilding it.** These are real GUI clients, so the build needs the period
toolchain — wxWidgets 2.8.12, Berkeley DB 4.8, Boost, and Satoshi's OpenSSL 0.9.8
`libeay32.dll` (its import lib is generated from `bitcoin-0.1.0/libeay32.dll`).
Build the shared toolchain once, then `make` in any client folder:

```sh
toolchain/build-toolchain.sh             # fetches + cross-builds wx 2.8 + BDB 4.8 + boost (~15 min)
cd irc/freenode/bitcoin-0.1.0 && ../../../toolchain/make.sh   # Satoshi's makefile; network baked in
cd ../../lfnet/bitcoin-0.1.3 && ../../../toolchain/make.sh
```

Two build fixes were needed for a modern MinGW-w64 + GCC 10 cross-compiler and
are applied automatically: Berkeley DB 4.8 renames its `__atomic_compare_exchange`
(now a GCC builtin), and wxWidgets 2.8's `filefn.h` needs `<direct.h>` for
`_mkdir`/`_rmdir`. The result is a static `bitcoin.exe` depending only on
`libeay32.dll` and XP system DLLs — the same dependency footprint as Satoshi's
original.

All four builds were verified end to end under Wine: each registers on its live
network, `JOIN #bitcoin`, `WHO`, and decodes the channel (the LFnet builds even
pick up real peer addresses). See the per-network notes below.

#### LFnet — still has live Bitcoin nodes

**LFnet (`irc.lfnet.org`) is still alive, still hosts `#bitcoin`, and still has
real Bitcoin nodes advertising there.** The `irc/lfnet` build registers with no
tricks and its `WHO` actually returns `u…` peer nicks, which it decodes to live
addresses — in our run (captured from the from-source build):

```
GOT WHO: [u9ueBogRwgiMGAn]  new  CAddress(158.140.180.83:8333)
GOT WHO: [u5Gsj76PRzZwVQ4]  new  CAddress(76.17.176.13:8333)
```

(Bitcoin Core's *BlueMatt* is even still sitting in the channel.) **Important
caveat — NAT:** *advertising* an address on IRC is not the same as being
*reachable* at it. Every node we saw advertised on LFnet was behind NAT and
**refused the connection** (`76.17.176.13:8333` and `158.140.180.83:8333` both did)
— so the discovery half works (the nick decodes to a real address) but the connect
half fails, exactly as it would have for an un-forwarded home node in 2009. A node
is only usable if it is actually reachable: port-forwarded, or on your own LAN. To
guarantee a connectable peer for the mining demo we therefore advertised one
ourselves — an IRC bot joining `#bitcoin` with a nick encoding a reachable address,
plus a listener there; the client decoded it, connected (`vNodes` non-empty), and
with *Generate Coins* on it mined. If you only want the original
2009 behaviour with no recompile at all, the same redirect works on the *stock*
unmodified client via `/etc/hosts` (LFnet speaks the old protocol, bare `\r` and
all):

```sh
echo "45.76.142.73 chat.freenode.net" | sudo tee -a /etc/hosts   # an irc.lfnet.org IP
# undo:  sudo sed -i '/45.76.142.73 chat.freenode.net/d' /etc/hosts
```

#### Freenode — reachable again, but four layers of drift

Getting a 2009 client onto **today's live Freenode** means peeling back **four**
separate incompatibilities. The `irc.cpp` fix handles the protocol ones; the
rest are about the network's modern anti-abuse:

1. **Line endings *(fixed in `irc.cpp`)*.** The client terminates every IRC line
   with a bare `\r`; Freenode's InspIRCd-3 requires `\r\n` and so never parses the
   client's `NICK`/`PONG` ("Registration timeout"). The `\r`→`\r\n` change above
   fixes it. (If you can't rebuild, the no-recompile alternatives are a one-byte
   patch of each terminator in the `.exe` — modern Freenode also accepts a bare
   `\n` — or a tiny MITM TCP proxy that rewrites the client→server stream.)

2. **Hostname NOTICE format *(usually a non-issue)*.** Before sending `NICK` the
   client blocks in `RecvUntil(... "Found your hostname", "using your IP address
   instead" ...)`. If your IP has reverse DNS, Freenode replies `*** Found your
   hostname (host) -- cached`, which still matches — so nothing is needed. Only
   if your IP has *no* rDNS does Freenode send `using your IP address (1.2.3.4)
   instead`, whose inserted IP breaks the exact-substring match and hangs the
   client; there a proxy has to translate that one NOTICE back to the literal
   string. Our residential IP had rDNS, so the rebuilt client sailed through.

3. **IP reputation.** Even with the protocol fixed, Freenode rejects many IPs.
   Our residential IP got **Z-lined** ("Suspected spam, temporary ban, 24 HRS")
   after too many connection attempts, and routing through **Cloudflare WARP made
   it worse** — those egress IPs are listed in **DroneBL** (a proxy/VPN/drone
   blocklist) and Freenode auto-Z-lines anything in it (`You are listed in
   DroneBL`). What worked was a **clean residential IP**: a router reconnect
   handed us a fresh dynamic IP (not banned, not in DroneBL) and registration
   went straight through — `PONG`, `001`…`004`, `JOIN`, `WHO`, full channel list,
   ChanServ welcome. (The Freenode build sidesteps the separate *nick* filter with
   its `x<digits>` nick, which carries no address for the filter to flag — see the
   next layer.)
   We later pinned this down to a pure **connection-rate** effect: from a fresh
   IP, a *single* client connection — no verification probes, no bot, no
   reconnect storm — registers and stays connected indefinitely (we held one open
   and watched it answer server `PING`s with no Z-line). Every ban we suffered was
   self-inflicted by our own volume of test connections in a short window; the
   discipline that works is **one connection and leave it alone**.

4. **The advertisement nick is itself banned — by *shape*, not by prefix.** A node
   advertises by setting its nick to the base58check of its address; Freenode's
   anti-spam **Z-lines any such nick** (`432 ... Suspected spam` at registration, or
   a `465` Z-line moments after `JOIN`), precisely because 2009-era Bitcoin clients
   once flooded it with exactly those nicks. We confirmed the consequence directly:
   of the **77** members our client saw in a `WHO`/`NAMES` of `#bitcoin`, **zero**
   decoded to an address and **all 77 were people** — and not random ones: Adam Back
   (`adam3us`), Luke Dashjr (`luke-jr`/`LukeDashjr`), Ruben Somsen (`RubenSomsen`),
   Lisa Neigut (`niftynei`), `shesek`, `jarolrod` and other Bitcoin developers.
   (Even ``Neo`Nemesis``, the anti-spam entity whose name appears in our `Z-lined`
   errors, sits in the channel — the guard that evicted the nodes is itself a
   member.) We tested whether a *different prefix* would dodge it — building a client
   that advertises with `n<base58check>` instead of `u<base58check>` — and ran a
   clean **side-by-side on one fresh IP**:
   - the **`x<digits>`** client (`x787746620`) registered, stayed connected, and
     **mined 4 blocks** (`height 0 → 4`) — no ban for the whole session;
   - then, on that **same** IP, an **`n<base58check>`** nick (`nC8r5KBgMr2ZkVP`)
     registered (`001`–`004`), `JOIN`ed `#bitcoin`, and was **Z-lined ~1 second
     after the JOIN**.

   Same clean IP, opposite outcomes — so the ban is triggered by the **nick**, and
   the filter targets the **base58-address *shape*** (a letter plus a long
   mixed-case blob), not the `u` character. The `n` prefix doesn't evade anything;
   it only delays the Z-line to just after `JOIN`. Only the addressless
   `x<digits>` form survives, which is why the Freenode build forces it
   (`-DIRC_FORCE_XNICK`). **You cannot advertise an address on Freenode at all.** For
   the **mining** demo we supply the peer ourselves — a held TCP socket to the
   client's own port, or a MITM proxy that injects a fabricated `JOIN` for a
   reachable address — after which, with *Generate Coins* on, the Freenode client
   mined those blocks on the January 2009 genesis (`proof-of-work found`).

**Bottom line:** neither IRC bootstrap is dead, but they fail differently.
**Freenode is reachable again** — with the `\r\n` fix, the `x<digits>` nick, and a
clean IP used sparingly, the `irc/freenode` build registers, joins, runs, and (with
a supplied peer) mines on the January 2009 genesis. But it can never **advertise or
discover**: every address-carrying nick is Z-lined, so its `#bitcoin` channel is
**77 people, not nodes**, and you must hand the client a peer yourself. (Freenode
also Z-lines by **IP connection rate**, so: one connection, left alone.) **LFnet, by
contrast, still does real peer discovery**: the `irc/lfnet` build reads genuine
`u<base58>` node advertisements out of the channel and decodes them to addresses —
except that every node we found there sat **behind NAT and refused connections**
(`76.17.176.13`, `158.140.180.83`), so discovery succeeds but the connect doesn't,
unless the node is actually reachable (port-forwarded, or on your LAN). In short:
Freenode = register and mine, but no advertising or discovery; LFnet = real
discovery, but the advertised peers are unreachable behind NAT.

**Worth stating precisely: Freenode's failure is a *policy* limit, not a protocol
one.** The client and the 2009 IRC bootstrap are fully capable of advertising and
discovering there — the `\r\n` fix already gets a node registered, joined and
reading `#bitcoin` — and the *only* remaining blocker is the anti-spam rule that
Z-lines the base58-address nick shape. We confirmed this is by **shape, not
prefix**: building a client that advertises with `n<base58check>` instead of
`u<base58check>` does **not** dodge it — on one clean IP, the `x<digits>` client
stayed connected and mined four blocks, while an `n<base58check>` nick registered,
`JOIN`ed, and was **Z-lined ~1s after the JOIN**. So the "n trick" only delays the
ban to just after `JOIN`; it never evades it. **If that rule were lifted — or the
same build were pointed at any network that permits the advertisement nick (LFnet
today, or a private/self-hosted IRC server you control) — the
`u<base58check>` advertise-and-discover path would light up unchanged.** This is the
relevant fact for anyone wanting to **revive the legacy IRC bootstrap**: nothing in
the client needs further repair; you only need a host network whose policy allows
the address nick.

#### Alternatives without recompiling

The prebuilt clients in `irc/` are the easy route, but every fix here can also be
applied to the **stock, unmodified** 0.1.0/0.1.3 `bitcoin.exe` without the
wxWidgets/BDB toolchain:

- **Switch IRC network — `/etc/hosts`.** LFnet speaks the 2009 protocol, so the
  original client just needs its hard-coded `chat.freenode.net` pointed at an
  LFnet IP (the `/etc/hosts` line shown above). No recompile, no binary edit.
- **Line endings — one-byte `.exe` patch.** Modern Freenode accepts a bare `\n`,
  so the five line terminators the client *sends* can be flipped `0x0D`→`0x0A`
  directly in the official binary: the four format strings `NICK %s\r`,
  `USER %s 8 * : %s\r`, `JOIN #bitcoin\r`, `WHO #bitcoin\r`, plus the `\r` the
  `PONG` builder appends (a `mov $0xd` immediate right after the `strLine[1]='O'`
  that turns `PING` into `PONG`). Five single-byte edits, no recompile.
- **Everything via a MITM TCP proxy.** Redirect `chat.freenode.net` to
  `127.0.0.1` (one `/etc/hosts` line) and run a tiny proxy that forwards to a real
  Freenode IP while: rewriting the client→server stream `\r`→`\r\n` (line
  endings); translating the hostname `NOTICE` back to the literal the 2009 client
  expects (only needed for IPs without rDNS); and **injecting** the fabricated
  `u<base58check>` peer `JOIN` toward the client (the only way to advertise a peer
  on Freenode, whose anti-spam bans that nick shape). This is what we used to take
  the **unmodified** client all the way to mining on Freenode.

#### Building the GUI from source — toolchain, errors, and the release/debug split

Recompiling 0.1.0/0.1.3 is harder than it looks: they are full **wxWidgets**
desktop apps, so the build needs a period GUI/database stack cross-compiled for
32-bit Windows. `toolchain/build-toolchain.sh` fetches and builds it; the pieces
and the snags are worth recording. The two library fixes below are kept as
ready-to-apply patches in `patches/compilation/` (the toolchain script applies
them automatically).

**The toolchain (all cross-built for `i686-w64-mingw32`):**

| Component | Version | Role | Notes |
|-----------|---------|------|-------|
| wxWidgets | 2.8.12  | GUI toolkit | built **ANSI** (not Unicode) + **static**, matching the `wxmsw28` (no `u`) names in Satoshi's makefile |
| Berkeley DB | 4.8.30.NC | wallet + block db (`db_cxx`) | C++ API, static |
| Boost | 1.55 | `foreach` / `lexical_cast` / `tuple` | **headers only** — Bitcoin links no Boost libs |
| OpenSSL | 0.9.8 | ECDSA / SHA / RIPEMD | the period `openssl/` headers in `toolchain/openssl/`, with a `libeay32.a` import lib generated from Satoshi's `libeay32.dll` |

**Build errors hit with a modern (GCC 10 / MinGW-w64) cross-compiler, and the fixes:**

- **Berkeley DB — `__atomic_compare_exchange` redefined.** BDB 4.8 ships its own
  function by that name, which is now a GCC built-in. Fix: rename it (to
  `__atomic_compare_exchange_db`) in `dbinc/atomic.h` — the long-known BDB-4.8
  patch.
- **wxWidgets — `_mkdir` / `_rmdir` not declared** (`filefn.cpp`). wx 2.8's
  `wxPOSIX_IDENT` macro picks the underscore (MSVC) spellings for everyone except
  `__MINGW64__`, but a 32-bit MinGW-w64 toolchain defines `__MINGW32__`, *not*
  `__MINGW64__`, and the underscore functions live in `<direct.h>` which the file
  never includes. Fix: `#include <direct.h>` under `#ifdef __MINGW32__` in
  `wx/filefn.h`.
- **`windres` chokes on `-mthreads`.** `wx-config --cxxflags` includes compiler
  flags the resource compiler rejects; each client `Makefile` filters the
  resource-compiler flags down to just `-I`/`-D`.
- **MinGW runtime DLLs at launch.** Satoshi's own `makefile` links the C/C++
  runtime *dynamically* (it has no `-static`), so the exe imports
  `libgcc_s_dw2-1.dll` and `libstdc++-6.dll` — the modern MinGW-w64 equivalents of
  the `mingwm10.dll` Satoshi shipped in 2009. Since we build from his unmodified
  makefile, each `irc/` client folder **ships those two DLLs** (stripped: ~0.1 MB
  and ~1.8 MB) next to `bitcoin.exe` and `libeay32.dll`, just as Satoshi shipped
  `mingwm10.dll` next to his.

**Release vs. debug, and where the log goes.** In `util.h`, `printf` is
`#define`d to `OutputDebugStringF`, and that function only opens **`debug.log`**
when `__WXDEBUG__` is defined — i.e. in a **debug** build. Satoshi's *distributed*
0.1.0/0.1.3 binaries were debug builds (that is why running them writes a
`debug.log` next to the exe). Our from-source binaries are **release** builds: we
must match the wxWidgets libraries, which are built `--disable-debug`, because a
`__WXDEBUG__` app linked against release wx would mismatch wx's object layout and
crash. A release build therefore writes **no `debug.log`**; its trace goes to
`stdout`, which under Wine is block-buffered into the redirect file and only
appears on flush — so during a run the IRC dialogue is invisible even though the
client is registered (the give-away is a live, *persistent* TCP connection to the
ircd: an unregistered client is dropped within a minute). To watch a from-source
build's IRC exchange live we pointed it at a logging TCP proxy instead.

**A startup delay we also fixed.** Before IRC, `StartNode` calls
`GetMyExternalIP`, which in the original code connects to hard-coded IPs the
IP-echo services had in 2008 (`72.233.89.199` for *whatismyip.com* in 0.1.0; two
of them in 0.1.3) and requests long-gone endpoints. The domains still work today
but moved off those IPs years ago, so each connect sits in `SYN-SENT` until it
times out (minutes under Wine) before the IRC thread even starts. The `irc/`
source resolves the services by **domain name** instead
(`gethostbyname("www.whatismyip.com")` etc., with a null-guard), so the dead IPs
are never dialled and the node reaches IRC in seconds. Even so, `GetMyExternalIP`
may now fail *fast* to parse a modern response — harmless, since its only use is to
pick the address the node advertises, which falls back to the LAN IP.

**Size.** The GUI links big (~12 MB with debug symbol tables). The committed
`bitcoin.exe` is run through `i686-w64-mingw32-strip` (a packaging step — it does
not touch the source or makefile), dropping it to ~6.4 MB, essentially the size of
Satoshi's original binary. Stripping removes only the symbol/line tables used by a
debugger; the machine code and behaviour are identical. The shipped runtime DLLs
are stripped the same way.

A curiosity: across these clients there are **two historical genesis blocks**.
0.1.0 and 0.1.3 load the official January 2009 genesis
(`000000000019d6…`, carrying the *Times* headline), while bitcoin-nov08-rebuild
reproduces Satoshi's pre-release September 2008 genesis (`000006b1…`, no
headline). Three codebases, two genesis blocks — the rebuild grows its chain
standalone (no peer gate), while 0.1.0/0.1.3 grow theirs only once you hand them
a peer (held socket, a second node, or the IRC route above).
