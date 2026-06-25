// nov08-rebuild - console startup.
//
// Replaces the original ui.cpp/init (not included in the Nov 2008 preview):
// it initializes, rebuilds the block index from the blk*.dat files, loads the
// wallet and addresses, brings up the P2P network and enters the main loop.
#include "headers.h"
#include <csignal>

// --- UI stub (the wxWidgets interface lived in ui.cpp, never released) ---
void MainFrameRepaint() { }

static void HandleSigInt(int)
{
    printf("\nShutting down...\n");
    fShutdown = true;
}

int main(int argc, char* argv[])
{
    setbuf(stdout, NULL);   // unbuffered console output
    printf("======================================================\n");
    printf(" Bitcoin - rebuild of Satoshi Nakamoto's preview\n");
    printf(" (Nov-2008 code, nov08-rebuild console build)\n");
    printf("======================================================\n");

    bool fGen = false;
    for (int i = 1; i < argc; i++)
        if (string(argv[i]) == "-gen")
            fGen = true;

    signal(SIGINT, HandleSigInt);

    // Randomness seed
    RandAddSeed(true);

    // 1) Block index (creates the genesis block if missing)
    printf("Loading block index...\n");
    if (!LoadBlockIndex(true))
    {
        printf("Error: LoadBlockIndex() failed\n");
        return 1;
    }
    printf("  blocks=%d  best_height=%d\n", (int)mapBlockIndex.size(), nBestHeight);

    // 2) Wallet. keyUser must hold a real key pair: the miner pays the coinbase
    // to keyUser.GetPubKey(), and the original code that set it lived in the
    // unreleased init/ui. Initialize it here (new key, or from the default key).
    printf("Loading wallet...\n");
    LoadWallet();
    if (mapKeys.empty())
    {
        keyUser.MakeNewKey();
        AddKey(keyUser);
        CWalletDB().WriteDefaultKey(keyUser.GetPubKey());
        printf("  generated default key (%d bytes of public key)\n", (int)keyUser.GetPubKey().size());
    }
    else
    {
        vector<unsigned char> vchDefaultKey;
        CWalletDB().ReadDefaultKey(vchDefaultKey);
        if (vchDefaultKey.empty() || !mapKeys.count(vchDefaultKey))
            vchDefaultKey = mapKeys.begin()->first;   // fall back to any key in the wallet
        keyUser.SetPrivKey(mapKeys[vchDefaultKey]);
    }
    printf("  keys=%d  transactions=%d\n", (int)mapKeys.size(), (int)mapWallet.size());

    // 3) Known peer addresses
    LoadAddresses();
    printf("  known addresses=%d\n", (int)mapAddresses.size());

    // 4) P2P network
    printf("Starting node (port %d)...\n", ntohs(DEFAULT_PORT));
    string strError;
    if (!StartNode(strError))
        printf("Warning: StartNode could not fully start: %s\n", strError.c_str());

    // 5) Optional mining
    if (fGen)
    {
        printf("Mining enabled (-gen): launching BitcoinMiner...\n");
        fGenerateBitcoins = true;
        _beginthread(ThreadBitcoinMiner, 0, NULL);
    }

    printf("Node running. Ctrl-C to exit.\n");
    while (!fShutdown)
        Sleep(500);

    StopNode();
    DBFlush(true);
    printf("Stopped cleanly.\n");
    return 0;
}
