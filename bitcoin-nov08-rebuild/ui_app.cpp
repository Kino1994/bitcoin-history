// nov08-full-rebuild - wxWidgets entry point and minimal GUI front-end.
//
// This is the glue the November 2008 preview never shipped: its ui.{h,cpp}
// (a wxWidgets app) were part of "the rest is coming soon" and are unrecoverable.
// Rather than back-port 0.1.0's full 3000-line ui.cpp (a large API-drift job),
// this is a small but REAL wxWidgets window driving the byte-exact nov08 core:
// it runs the same boot sequence as the console init.cpp, then shows live node
// status (genesis hash, block height, peers, wallet) and a Mine toggle.
//
// The full 0.1.0 UI port remains the next increment (see README).

#include "headers.h"            // nov08 core: types, globals, windows/winsock setup

// wx pulls in <windows.h>; the core already did, with WIN32_LEAN_AND_MEAN.
// Drop the core's min/max function-macros so wx's templates compile.
#undef min
#undef max
#include <wx/wx.h>

// ---- core entry points we reuse (all defined in the nov08 core) ----
//   LoadBlockIndex / LoadWallet / LoadAddresses / StartNode / StopNode
//   nBestHeight, pindexBest, mapBlockIndex, mapKeys, keyUser, vNodes, ...

extern uint256 hashTimeChainBest;   // nov08 core's best-block hash (main.cpp)

static const int ID_TIMER = 1001;
static const int ID_MINE  = 1002;

class CMainFrame : public wxFrame
{
public:
    bool        fMining;
    bool        fMinerStarted;
    wxStaticText* pGenesis;
    wxStaticText* pHeight;
    wxStaticText* pPeers;
    wxStaticText* pWallet;
    wxStaticText* pAddress;
    wxStaticText* pBestHash;
    wxButton*     pMineBtn;
    wxTimer       timer;

    CMainFrame()
        : wxFrame(NULL, wxID_ANY,
                  _T("Bitcoin - nov08-full-rebuild (wxWidgets)"),
                  wxDefaultPosition, wxSize(560, 320)),
          fMining(false), fMinerStarted(false), timer(this, ID_TIMER)
    {
        wxPanel* panel = new wxPanel(this);
        wxBoxSizer* col = new wxBoxSizer(wxVERTICAL);

        wxFont mono(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

        wxStaticText* title = new wxStaticText(panel, wxID_ANY,
            _T("Satoshi's November 2008 preview - reconstructed core, wx front-end"));
        col->Add(title, 0, wxALL, 8);

        pGenesis  = new wxStaticText(panel, wxID_ANY, _T("genesis : -"));
        pBestHash = new wxStaticText(panel, wxID_ANY, _T("best    : -"));
        pHeight   = new wxStaticText(panel, wxID_ANY, _T("height  : -"));
        pPeers    = new wxStaticText(panel, wxID_ANY, _T("peers   : -"));
        pWallet   = new wxStaticText(panel, wxID_ANY, _T("wallet  : -"));
        pAddress  = new wxStaticText(panel, wxID_ANY, _T("address : -"));
        pGenesis->SetFont(mono);  pBestHash->SetFont(mono);
        pHeight->SetFont(mono);   pPeers->SetFont(mono);
        pWallet->SetFont(mono);   pAddress->SetFont(mono);
        col->Add(pGenesis,  0, wxLEFT|wxRIGHT|wxTOP, 8);
        col->Add(pBestHash, 0, wxLEFT|wxRIGHT, 8);
        col->Add(pHeight,   0, wxLEFT|wxRIGHT, 8);
        col->Add(pPeers,    0, wxLEFT|wxRIGHT, 8);
        col->Add(pWallet,   0, wxLEFT|wxRIGHT, 8);
        col->Add(pAddress,  0, wxLEFT|wxRIGHT, 8);

        pMineBtn = new wxButton(panel, ID_MINE, _T("Start mining (-gen)"));
        col->Add(pMineBtn, 0, wxALL, 8);

        panel->SetSizer(col);
        Refresh1();
        timer.Start(1000);   // poll the core once a second (UI thread only)
    }

    void Refresh1()
    {
        // nov08's CBlockIndex stores no hash (0.1.0 added phashBlock); use the
        // globals the core keeps instead: hashGenesisBlock and hashTimeChainBest.
        pGenesis->SetLabel(wxString::Format(_T("genesis : %s"),
            hashGenesisBlock.ToString().c_str()));
        if (pindexBest)
            pBestHash->SetLabel(wxString::Format(_T("best    : %s"),
                hashTimeChainBest.ToString().c_str()));
        pHeight->SetLabel(wxString::Format(_T("height  : %d   (blocks=%d)"),
            nBestHeight, (int)mapBlockIndex.size()));
        int nPeers = 0;
        CRITICAL_BLOCK(cs_vNodes) nPeers = (int)vNodes.size();
        pPeers->SetLabel(wxString::Format(_T("peers   : %d connected   (known=%d)"),
            nPeers, (int)mapAddresses.size()));
        pWallet->SetLabel(wxString::Format(_T("wallet  : %d key(s)"), (int)mapKeys.size()));
        if (!keyUser.GetPubKey().empty())
            pAddress->SetLabel(wxString::Format(_T("address : %s"),
                PubKeyToAddress(keyUser.GetPubKey()).c_str()));
    }

    void OnTimer(wxTimerEvent&) { Refresh1(); }

    void OnMine(wxCommandEvent&)
    {
        fMining = !fMining;
        fGenerateBitcoins = fMining;
        if (fMining && !fMinerStarted)
        {
            fMinerStarted = true;
            _beginthread(ThreadBitcoinMiner, 0, NULL);
        }
        pMineBtn->SetLabel(fMining ? _T("Stop mining") : _T("Start mining (-gen)"));
    }

    void OnClose(wxCloseEvent&)
    {
        timer.Stop();
        fGenerateBitcoins = false;
        fShutdown = true;
        StopNode();
        DBFlush(true);
        Destroy();
    }

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CMainFrame, wxFrame)
    EVT_TIMER(ID_TIMER, CMainFrame::OnTimer)
    EVT_BUTTON(ID_MINE, CMainFrame::OnMine)
    EVT_CLOSE(CMainFrame::OnClose)
END_EVENT_TABLE()

CMainFrame* pframeMain = NULL;

// Called by the core (main.cpp / node.cpp) when state changes. Runs on worker
// threads, so it must NOT touch wx widgets directly; the 1 Hz timer repaints.
void MainFrameRepaint() { }

class CMyApp : public wxApp
{
public:
    bool OnInit()
    {
        // ---- same boot sequence as the console init.cpp ----
        RandAddSeed(true);

        if (!LoadBlockIndex(true)) {           // creates byte-exact genesis if missing
            wxMessageBox(_T("LoadBlockIndex() failed"));
            return false;
        }

        LoadWallet();
        if (mapKeys.empty()) {
            keyUser.MakeNewKey();
            AddKey(keyUser);
            CWalletDB().WriteDefaultKey(keyUser.GetPubKey());
        } else {
            vector<unsigned char> vchDefaultKey;
            CWalletDB().ReadDefaultKey(vchDefaultKey);
            if (vchDefaultKey.empty() || !mapKeys.count(vchDefaultKey))
                vchDefaultKey = mapKeys.begin()->first;
            keyUser.SetPrivKey(mapKeys[vchDefaultKey]);
        }

        LoadAddresses();

        string strError;
        StartNode(strError);                   // P2P threads on port 2222

        pframeMain = new CMainFrame();
        pframeMain->Show();
        SetTopWindow(pframeMain);
        return true;
    }
};

IMPLEMENT_APP(CMyApp)
