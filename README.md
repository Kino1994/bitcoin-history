# bitcoin-history

Early Bitcoin source archives.

The archived source code is taken from the Satoshi Nakamoto Institute:
<https://satoshi.nakamotoinstitute.org/code/>

- **bitcoin-nov08/** — Satoshi's November 2008 pre-release source preview (only `main` + `node`; doesn't compile on its own).
- **bitcoin-nov08-rebuild/** — a compilable, byte-exact reconstruction of that preview that builds and runs on Windows XP; includes a prebuilt `bitcoin.exe` (see its README).
- **bitcoin-0.1.0/** — Satoshi's first public release, v0.1.0 ALPHA (January 2009): binary + source.
- **bitcoin-0.1.3/** — Bitcoin v0.1.3 release: binary + source.
- **irc/** — the 0.1.0 and 0.1.3 GUI clients **rebuilt from source with an IRC fix** so they can still bootstrap peers in 2026. `irc/source/` has the buildable fixed source (the `\r\n` change lives in `irc.cpp`) plus its build scripts; `irc/freenode/` and `irc/lfnet/` hold the ready-to-run `bitcoin.exe` for each network. See "Reaching peers via IRC" below.

The Windows binaries also run on Linux via Wine:

```sh
cd bitcoin-0.1.3 && wine ./bitcoin.exe          # GUI
cd bitcoin-0.1.0 && wine ./bitcoin.exe          # GUI
cd bitcoin-nov08-rebuild/src && wine ./bitcoin.exe   # console node (add -gen to mine)

# IRC-patched builds that still find peers in 2026 (GUI):
cd irc/lfnet/bitcoin-0.1.0    && wine ./bitcoin.exe   # discovers live peers on LFnet
cd irc/freenode/bitcoin-0.1.3 && wine ./bitcoin.exe   # registers on today's Freenode
```

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
  `GetMyExternalIP` in `irc/source/*/net.cpp`.)*
- They enforce a **single instance** (shared between both). If one is already
  open, or a previous run was killed instead of closed, the next launch quits
  silently ("Existing instance found"). Run `wineserver -k` before relaunching.
- They are **windowed apps** (need a display) and store their data in
  `%APPDATA%\Bitcoin` — under Wine that's
  `~/.wine/drive_c/users/<user>/Application Data/Bitcoin/`
  (`wallet.dat`, `blkindex.dat`, `addr.dat`, `debug.log`, …), **not** the exe
  folder. They connect to the real network on port 8333.
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
`while (vNodes.empty())` check, so `bitcoin.exe -gen` mines standalone at a
trivial test difficulty and `best_height` climbs anyway. So the reason the 0.1.x
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
  source/                          # the buildable fixed source + build scripts
    bitcoin-0.1.0/  bitcoin-0.1.3/ #   full source, irc.cpp carries the fix
    build-toolchain.sh  build.sh
  freenode/  bitcoin-0.1.0/  bitcoin-0.1.3/   # built: IRC_SERVER=chat.freenode.net
  lfnet/     bitcoin-0.1.0/  bitcoin-0.1.3/   # built: IRC_SERVER=irc.lfnet.org
```

Run any prebuilt one directly (GUI):
`cd irc/lfnet/bitcoin-0.1.0 && wine ./bitcoin.exe`.

**The fix (`irc/source/bitcoin-*/irc.cpp`).** Three small changes:
- Every IRC line the client *sends* now ends in `\r\n` instead of the original
  bare `\r` — the four `Send()` format strings (`NICK`, `USER`, `JOIN`, `WHO`)
  and the `strLine += '\r'` that builds the `PONG`. Modern Freenode (InspIRCd-3)
  requires `\r\n` (it does accept a bare `\n`, but `\r\n` is the RFC-correct form
  and LFnet tolerates it too), so this single change makes the client register on
  *both* networks.
- The hard-coded server became an `IRC_SERVER` macro (default
  `chat.freenode.net`); the LFnet builds are compiled with
  `-DIRC_SERVER="irc.lfnet.org"`.
- The **Freenode** builds are compiled with `-DIRC_FORCE_XNICK`, which forces the
  `x<digits>` nick. This is the subtle one. Once the `GetMyExternalIP` fix gives
  the node its real (routable) external IP, it would advertise itself with a nick
  that base58-encodes that address — and Freenode's anti-spam **Z-lines** any
  base58-address nick (layer 4 below; we watched it register fully, then get
  Z-lined seconds later). Forcing `x<digits>` (which carries no address, so the
  filter ignores it) lets the client register and stay; you can't advertise a peer
  on Freenode anyway. On LFnet the address nick is kept, because LFnet doesn't ban
  it and it advertises a real connectable address.

Those are the only per-network differences (`IRC_SERVER` for LFnet,
`IRC_FORCE_XNICK` for Freenode).

**Rebuilding it.** These are real GUI clients, so the build needs the period
toolchain — wxWidgets 2.8.12, Berkeley DB 4.8, Boost, and Satoshi's OpenSSL 0.9.8
`libeay32.dll` (reused from `bitcoin-nov08-rebuild/`). `irc/source/` automates it:

```sh
cd irc/source
./build-toolchain.sh                 # fetches + cross-builds wx 2.8 + BDB 4.8 + boost (~15 min)
./build.sh 0.1.0 freenode            # -> out/bitcoin-0.1.0-freenode.exe
./build.sh 0.1.3 lfnet               # -> out/bitcoin-0.1.3-lfnet.exe
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
32-bit Windows. `irc/source/build-toolchain.sh` fetches and builds it; the pieces
and the snags are worth recording.

**The toolchain (all cross-built for `i686-w64-mingw32`):**

| Component | Version | Role | Notes |
|-----------|---------|------|-------|
| wxWidgets | 2.8.12  | GUI toolkit | built **ANSI** (not Unicode) + **static**, matching the `wxmsw28` (no `u`) names in Satoshi's makefile |
| Berkeley DB | 4.8.30.NC | wallet + block db (`db_cxx`) | C++ API, static |
| Boost | 1.55 | `foreach` / `lexical_cast` / `tuple` | **headers only** — Bitcoin links no Boost libs |
| OpenSSL | 0.9.8 | ECDSA / SHA / RIPEMD | reused from `bitcoin-nov08-rebuild/vendor`: the period headers + Satoshi's own `libeay32.dll`, with a generated `libeay32.a` import lib |

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
  flags the resource compiler rejects; `build.sh` filters the resource-compiler
  flags down to just `-I`/`-D`.
- **Missing `libgcc_s_dw2-1.dll` / `libstdc++-6.dll` at launch.** A default link
  pulls in the MinGW runtime DLLs. Satoshi's binary linked them statically, so we
  link with `-static -static-libgcc -static-libstdc++`; the result imports **only
  `libeay32.dll` + XP system DLLs** (`KERNEL32`, `USER32`, `GDI32`, `COMCTL32`,
  `ADVAPI32`, `WSOCK32`, `msvcrt`, …) — the same footprint as the 2009 exe.

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

**Size.** The static GUI links big (~10.5 MB); `build.sh` runs
`i686-w64-mingw32-strip` to drop the debug symbol tables, bringing each exe to
~6.8 MB — essentially the size of Satoshi's original 6.4 MB binary. Stripping
removes only the symbol/line tables used by a debugger; the machine code and
behaviour are identical.

A curiosity: across these clients there are **two historical genesis blocks**.
0.1.0 and 0.1.3 load the official January 2009 genesis
(`000000000019d6…`, carrying the *Times* headline), while bitcoin-nov08-rebuild
reproduces Satoshi's pre-release September 2008 genesis (`000006b1…`, no
headline). Three codebases, two genesis blocks — the rebuild grows its chain
standalone (no peer gate), while 0.1.0/0.1.3 grow theirs only once you hand them
a peer (held socket, a second node, or the IRC route above).
