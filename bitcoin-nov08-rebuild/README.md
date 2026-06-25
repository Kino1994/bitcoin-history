# bitcoin-nov08-rebuild

A **compilable and linkable** reconstruction of the source-code preview that
Satoshi Nakamoto released in **November 2008** (`../bitcoin-nov08/`), aimed at
producing a native **Windows XP** executable.

The original preview only contained `main.{h,cpp}` and `node.{h,cpp}` ("These are
just the main files. The rest is coming soon.") and therefore **did not compile**:
it was missing the support modules, the database layer and the entry point, and
it depended on wxWidgets, Boost and Berkeley DB. This project rebuilds the
minimum needed for the core (consensus + P2P network) to compile and link.

## Background & references

- **The source code** — Satoshi shared this preview on request in November 2008,
  before the January 2009 launch. It survived because *Cryddit* posted it on the
  BitcoinTalk forum in 2013:
  <https://bitcointalk.org/index.php?topic=382374.0>
- **The story of this genesis block** — a good narrative on the pre-release
  ("alternative") genesis block `000006b1…`, its September 10 2008 timestamp and
  its abandonment before the official Times-headline genesis:
  <https://serhack.me/articles/story-behind-alternative-genesis-block-bitcoin/>

  That article refers to the script opcodes by name; the byte values that
  actually matter for the genesis hash (`OP_CODESEPARATOR`=0xa9,
  `OP_CHECKSIG`=0xaa in nov08) are what this reconstruction recovers — see
  "Byte-exact genesis".

## Result

`src/bitcoin.exe` → `PE32 executable (console) Intel 80386, for MS Windows`,
**Windows CUI 4.0** subsystem (XP-compatible). It depends only on `libeay32.dll`
(the OpenSSL 0.9.8 that Satoshi shipped) and on XP system DLLs (`KERNEL32`,
`WS2_32`, `ADVAPI32`, `msvcrt`). libgcc/libstdc++ are linked statically.

## How it is built

```sh
cd src
make implib   # generate vendor/libeay32.a from Satoshi's libeay32.dll (one time)
make xp       # cross-compile bitcoin.exe with MinGW i686
```

Requirements: `g++-mingw-w64-i686`, `mingw-w64-tools` (for `gendef`).

## How to run it on Windows XP

Copy **two files** to the XP machine, in the same folder, and run:

```
bitcoin.exe
libeay32.dll        (from bitcoin-0.1.0/ — the one Satoshi shipped)
```

`bitcoin.exe` starts a console node: it rebuilds the block index from
`blk*.dat` (creating the genesis block if missing), loads/creates the wallet
(`wallet.dat`), loads addresses (`addr.dat`) and brings up the P2P network on
port **2222** (nov08's original port). `bitcoin.exe -gen` also mines.
Press `Ctrl-C` to stop.

> Warning: this is a 2008 node with test difficulty (`MINPROOFOFWORK=20`) and the
> protocol of that era; it does **not** sync with today's Bitcoin network. It is
> for studying and running the historical code, ideally in isolation.

### Running it on Linux (under Wine)

```sh
cd src
WINEDEBUG=-all wine ./bitcoin.exe   # add -gen to mine; Ctrl-C to stop
```

### Data files / starting from scratch

The node keeps its state in four files **in the folder it runs from**:

| File | Contents |
|------|----------|
| `blk0001.dat` | the blocks (including the genesis block) |
| `blkindex.dat` | transaction index (TxPos) |
| `wallet.dat` | **your wallet (keys)** |
| `addr.dat` | known peer addresses |

To wipe everything and start fresh (next launch recreates the genesis block and a
brand-new key), delete those files plus any `.tmp`:

```sh
# Linux / Wine (from src/):
rm -f *.dat *.tmp
```
```
REM Windows XP (in the exe's folder):
del blk0001.dat blkindex.dat wallet.dat addr.dat *.tmp
```

> Deleting `wallet.dat` discards your keys irreversibly. To reset only the chain
> while keeping the wallet, delete `blk0001.dat`, `blkindex.dat` and `addr.dat`
> but keep `wallet.dat`.

## What changed relative to the preview (and why)

Every change is commented in the code with the `nov08-rebuild` prefix.

- **Support modules** brought in from `bitcoin-0.1.0/src` (the ones nov08 does
  not replace): `serialize.h`, `uint256.h`, `util.{h,cpp}`, `bignum.h`, `key.h`,
  `script.{h,cpp}`, `sha.{h,cpp}`, `base58.h`.
- **Rebuilt database layer** (`db.h`, `db.cpp`): nov08 used an index by
  transaction **position** (`ReadTxPos`/`WriteTxPos`/`EraseTxPos`/`ContainsTx`/
  `ReadDiskTx`/`ReadOwnerTxes`), not the later `CTxIndex` of 0.1.0. The Berkeley
  DB backend is replaced by an in-memory key/value store persisted to a flat file
  (same `CDataStream` serialization).
- **`headers.h`** console version + **`compat.h`** (Win32↔POSIX layer, only
  active off Windows, for native verification) + local **`winsock2.h`** shim.
- **`init.cpp`**: console `main()` entry point replacing the unreleased
  `ui.cpp`/init. It also initializes the global `keyUser` (a fresh key, or the
  stored default key) — the miner pays the coinbase to `keyUser.GetPubKey()`, and
  the code that set it lived in the unreleased init, so without this the miner
  dereferenced a null EC key. With it, `bitcoin.exe -gen` mines at the test
  difficulty and the chain grows and persists across restarts.
- **`market.{h,cpp}`**: inert *stubs* of the market subsystem (`mapProducts`,
  `mapTables`, `CTable`, `CProduct`, `AddAtomsAndPropagate`, `AdvertRemoveSource`)
  and of the UI callback `MainFrameRepaint`, which the preview references but
  whose code Satoshi never released. Also the bodies of
  `CTransaction::ClientConnectInputs` and `CNode::CancelSubscribe`.
- **`script.cpp`**: `SignatureHash` and `SignSignature` adapted to nov08's
  transaction layout (`nSequence` lives in `CTxOut`, not `CTxIn`; signing is done
  by the position of the spent output).
- **Boost** removed (`is_fundamental`→`std`, `BOOST_FOREACH`→range-for) and
  **wxWidgets** removed (logging/dialogs→console).
- **OpenSSL**: compiled against the period **0.9.8** headers (vendored in
  `vendor/openssl/`, 32-bit config) and linked against the original
  `libeay32.dll`, so `bignum.h`/`key.h` are left **unmodified**.
- **`script.h` opcode renumbering** (`OP_CODESEPARATOR`=0xa9, `OP_CHECKSIG`=0xaa):
  see "Byte-exact genesis" below.

## Byte-exact genesis (verified)

The genesis block is reproduced **byte-for-byte**: at startup the node computes

```
hashMerkleRoot = 769a5e93fac273fd825da42d39ead975b5d712b2d50953f35a4fdebdec8083e3
block hash     = 000006b15d1327d67e971d1de9116bd60a3a01556c91b6ebaa416ebc0cfaa646
```

both of which match the values Satoshi hard-coded, so his own genesis integrity
`assert`s pass.

Getting there required one real consensus finding. With 0.1.0's `script.h` the
merkle root came out wrong. The cause: between nov08 and 0.1.0 Satoshi inserted
`OP_HASH160` (0xa9) and `OP_HASH256` (0xaa) into the opcode enum, which shifted
`OP_CODESEPARATOR` 0xa9→0xab and `OP_CHECKSIG` 0xaa→0xac. nov08's genesis
`scriptPubKey` is `OP_CODESEPARATOR <pubkey> OP_CHECKSIG`, so those two bytes
changed the transaction hash. The fix (in `script.h`) keeps the `OP_HASH160/256`
names but moves them to 0xae/0xaf, restoring `OP_CODESEPARATOR`=0xa9 and
`OP_CHECKSIG`=0xaa. (The genesis block-header preimage that confirmed the target
merkle root was cross-checked against a value posted in a public r/btc thread.)

## Verified at runtime (under Wine)

```
Loading block index...
  blocks=1  best_height=0
Loading wallet...
  generated default key (65 bytes of public key)
Starting node (port 2222)...
bound to addrLocalHost = 192.168.1.67:2222
ThreadSocketHandler / ThreadOpenConnections / ThreadMessageHandler started
```

The node creates the genesis block, generates an ECDSA key via `libeay32.dll`,
brings up the three network threads on port 2222, and persists `blk0001.dat` and
`wallet.dat`.

## Honest limitations

- The owner index (`CTxDB::ReadOwnerTxes`) returns "not found": that index lived
  in the unreleased `db.cpp`; it only affects the SPV query for transactions by
  key.
- The `SIGHASH_SINGLE`/`SIGHASH_NONE` branches are approximated (the wallet uses
  `SIGHASH_ALL`).
- The market subsystem is inert (empty maps).
- It does not sync with today's Bitcoin network (2008 protocol, test difficulty).
