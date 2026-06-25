// nov08-rebuild - umbrella header (console version, no wxWidgets)
// Rebuilt from bitcoin-0.1.0's headers.h, adapted to the November 2008 preview
// (Satoshi's main/node) and to a console build.
#ifdef _MSC_VER
#pragma warning(disable:4786)
#pragma warning(disable:4804)
#pragma warning(disable:4717)
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0400          // target: Windows NT/2000/XP
#define WIN32_LEAN_AND_MEAN 1

// --- Win32<->POSIX compatibility (only active off Windows) ---
#include "compat.h"

// --- OpenSSL (crypto: SHA-256, RIPEMD-160, BIGNUM, ECDSA) ---
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>

// --- Networking ---
#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>
#include <process.h>
#include <io.h>
#else
#include "winsock2.h"   // local shim -> BSD sockets
#endif

// --- C standard ---
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <assert.h>
#include <memory>

// --- STL ---
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <algorithm>
#include <numeric>

using namespace std;

// On Windows, <windows.h> defines min/max as type-agnostic macros that Satoshi's
// code relies on. Off Windows we replicate them HERE (after the STL, so their
// headers aren't broken; the bitcoin code uses neither std::min nor
// numeric_limits::max, so this is safe).
#if !defined(WIN32) && !defined(_WIN32)
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

// --- Support modules (dependency order) ---
#include "serialize.h"
#include "uint256.h"
#include "util.h"
#include "key.h"
#include "bignum.h"
#include "base58.h"
#include "script.h"
#include "db.h"        // rebuilt for nov08's TxPos scheme
#include "market.h"    // stubs for unreleased pieces (market, UI)
#include "node.h"      // P2P network (nov08, Satoshi) - before main.h (defines CNode, pchMessageStart)
#include "main.h"      // bitcoin system (nov08, Satoshi)
