# patches/

Two kinds of patch live here, kept deliberately separate:

- **`code/`** — *original-code* patches: pure source diffs that turn one
  historical Bitcoin codebase into another (e.g. Nov-2008 preview → 0.1.0). They
  are **agnostic of how you compile** — they only describe the differences in
  Satoshi's (or the rebuild's) source. Whoever applies one then deals with the
  build on their own.

- **`compilation/`** — *library* patches: modern-toolchain fixes for the period
  dependencies (wxWidgets 2.8.12, Berkeley DB 4.8) so they cross-compile with a
  current GCC / MinGW-w64. These touch third-party libraries, **not** Bitcoin.

This split is intentional: a code patch never carries a build workaround, and a
compilation patch never carries a behavioural change.

---

## code/  — version-to-version source diffs

| Patch | From → To | Notes |
|-------|-----------|-------|
| `nov08-to-rebuild.patch`      | Nov-2008 preview → rebuild      | adds the support modules, flat-file DB and the `ui_app.cpp` wxWidgets GUI front-end |
| `nov08-to-0.1.0.patch`        | Nov-2008 preview → 0.1.0        | Satoshi's first public release (large: adds net/irc/ui/market, renumbers opcodes) |
| `0.1.0-to-0.1.3.patch`        | 0.1.0 → 0.1.3                   | Satoshi's own changes between the two releases |
| `freenode/0.1.0.patch`        | 0.1.0 → 0.1.0 IRC, **Freenode** | IRC fix + Freenode baked in |
| `freenode/0.1.3.patch`        | 0.1.3 → 0.1.3 IRC, **Freenode** | IRC fix + Freenode baked in |
| `lfnet/0.1.0.patch`           | 0.1.0 → 0.1.0 IRC, **LFnet**     | IRC fix + LFnet baked in |
| `lfnet/0.1.3.patch`           | 0.1.3 → 0.1.3 IRC, **LFnet**     | IRC fix + LFnet baked in |

Each diff is source-only (`.cpp`/`.h`): binaries, the vendored `openssl/`
headers, build files (`Makefile`/`makefile`), resources (`*.rc`, `rc/`) and docs
are excluded. New files appear against `/dev/null`.

**Apply** (onto a copy of the "From" tree):

```sh
# e.g. turn a pristine 0.1.0 source tree into the LFnet IRC client:
cd bitcoin-0.1.0/src
patch -p1 < ../../patches/code/lfnet/0.1.0.patch
```

All eight apply cleanly with `-p1` onto their "From" tree (verified). The two big
cross-version diffs (`nov08-to-0.1.0`, and the rebuild ones) are primarily
**reference** diffs — use them to read what changed; they apply onto the bare
"From" source but you then supply the build yourself.

### freenode vs lfnet — what the IRC patches contain

Each IRC patch is **self-contained**: it carries the whole IRC fix *and* bakes
the network choice into the source, so the result builds with **no `-D` flags**.
The IRC fix itself (identical in all four) is two files:

- **`irc.cpp`** — the `\r\n` line terminator (modern ircd needs it; the original
  sent a bare `\r`), the `IRC_SERVER` selection, the forced-nick logic, and
  comments explaining each.
- **`net.cpp`** — the external-**IP-check** change (resolve the discovery host by
  name instead of the dead hard-coded IPs, so `GetMyExternalIP` no longer stalls
  startup).

The **only** difference between the freenode and lfnet patch for a given version
is one injected line near the top of `irc.cpp`:

- **freenode**: `#define IRC_FORCE_XNICK` — server defaults to `chat.freenode.net`;
  forces an `x<digits>` nick (Freenode Z-lines any nick that base58-encodes an
  address).
- **lfnet**: `#define IRC_SERVER "irc.lfnet.org"` — keeps the original nick;
  LFnet still hosts live Bitcoin nodes.

Keeping them as four separate patches (rather than one patch + a build flag) is
deliberate: the network difference is then **localized and visible in the diff**,
and each patch reaches a complete, buildable version on its own. In fact the
committed `irc/<network>/bitcoin-<version>/` folders **are** the applied result of
these patches — flat source (**Satoshi's own `makefile` included**) + the built
`bitcoin.exe`. So applying a patch reproduces exactly that folder's source. To
build: run `toolchain/build-toolchain.sh` once, then `../../../toolchain/make.sh`
inside the folder (it drives Satoshi's unmodified makefile).

---

## compilation/  — period-library build fixes (modern GCC / MinGW-w64)

| Patch | Library | Needed by |
|-------|---------|-----------|
| `wxWidgets-2.8.12-mingw.patch`        | wxWidgets 2.8.12 | every GUI build (the nov08 rebuild, IRC clients) |
| `berkeley-db-4.8-gcc-atomic.patch`    | Berkeley DB 4.8  | only builds that link Berkeley DB (IRC clients; "purist" rebuild) |

**Apply** (inside each library's own source tree):

```sh
patch -p1 < /path/to/patches/compilation/wxWidgets-2.8.12-mingw.patch        # in wxWidgets-2.8.12/
patch -p1 < /path/to/patches/compilation/berkeley-db-4.8-gcc-atomic.patch    # in db-4.8.30.NC/
```

`toolchain/build-toolchain.sh` applies both automatically.

### The GCC-10 narrowing ("DWORD") issue — not a patch

Building wxWidgets 2.8.12 under GCC 10 can also fail on a `case -1:` over a
`DWORD` in `src/msw/utilsexc.cpp` (narrowing). This is **not** a wxWidgets bug:
it only fires because GCC 10 defaults to a newer C++ standard. Build wxWidgets in
its **native standard** and it disappears with no source edit and no flag hack:

```sh
CXXFLAGS="-std=gnu++98 -fpermissive -w"
```

(Verified: wxWidgets 2.8.12 builds clean this way needing only the `_mkdir`
patch above.) `-Wno-narrowing` is a fallback only if you insist on a newer
`-std`. The header of `wxWidgets-2.8.12-mingw.patch` documents this too.
