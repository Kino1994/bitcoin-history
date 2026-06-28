# bitcoin-0.1.3 — IRC-fixed client (Freenode)

Satoshi's Bitcoin **0.1.3** GUI, rebuilt from source with an IRC fix so it still
bootstraps peers in 2026 on **Freenode** (`chat.freenode.net`). This folder is *generated*
(see below) — the untouched original is `../../../bitcoin-0.1.3/`.

## How this folder was generated

It is exactly **`bitcoin-0.1.3/src` + `patches/code/freenode/0.1.3.patch`** applied.
The patch carries the IRC fix (the `\r\n` line terminator and the
`GetMyExternalIP` domain-name fix) and **bakes the network into `irc.cpp`** so it
builds with no `-D` flags:

```cpp
#define IRC_FORCE_XNICK        // server defaults to chat.freenode.net
```

On Freenode the client is forced to an `x<digits>` nick (`IRC_FORCE_XNICK`), because Freenode Z-lines any nick that base58-encodes an address. So it registers and can mine, but it cannot *advertise* a peer there.

To regenerate this source from a pristine tree:

```sh
cd bitcoin-0.1.3/src && patch -p1 < ../../patches/code/freenode/0.1.3.patch
```

## How it was built

From **Satoshi's own `makefile`, unchanged**, via the shared toolchain:

```sh
../../../toolchain/build-toolchain.sh   # one time: wx 2.8 + Berkeley DB 4.8 + Boost + libeay32.a
../../../toolchain/make.sh              # here -> stripped bitcoin.exe + runtime DLLs at this root
```

`make.sh` only adapts the *environment* (cross-`g++` driven in `-std=gnu++98`,
toolchain include/lib paths); it does not edit the makefile or the source. The
makefile links the C/C++ runtime **dynamically**, so this folder ships
`libgcc_s_dw2-1.dll` + `libstdc++-6.dll` next to `bitcoin.exe` and `libeay32.dll`
(the modern equivalent of the `mingwm10.dll` Satoshi shipped). Full toolchain
details — and how it differs from Satoshi's 2009 one — are in the root `README.md`.

## Running it

```sh
WINEDEBUG=-all wine ./bitcoin.exe
```

It registers on Freenode and looks for peers. (The window can look frozen /
"Not Responding" for a minute at startup — let it wait.)

**Log / data location.** This is a **release** build, so it writes **no
`debug.log`**; its trace goes to `stdout` (block-buffered under Wine, so it only
appears on flush). The wallet, chain and peer files live in **`%APPDATA%\Bitcoin`**
— under Wine `~/.wine/drive_c/users/$USER/Application Data/Bitcoin/`
(`wallet.dat`, `blkindex.dat`, `addr.dat`, the Berkeley DB env), **not** in this
folder.

See the root `README.md` ("Reaching peers via IRC") for the full IRC story and the
per-network drift, and `patches/README.md` for the patch layout.
