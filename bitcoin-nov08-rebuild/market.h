// nov08-rebuild - inert stubs for the pieces the November 2008 preview
// references but whose code Satoshi NEVER released ("the rest is coming soon"):
// the market subsystem (products / tables / atoms) and the UI callback. They are
// provided empty so the core (consensus + P2P network) compiles and runs; the
// maps are always empty, so the behavior is null.
#ifndef BITCOIN_MARKET_STUB_H
#define BITCOIN_MARKET_STUB_H

class CNode;

class CProduct { };
class CTable   { };

extern map<uint256, CProduct> mapProducts;
extern map<uint256, CTable>   mapTables;
extern CCriticalSection cs_mapProducts;
extern CCriticalSection cs_mapTables;

// Source advertising (broadcast): generic no-op for CProduct/CTable.
template<typename T>
inline void AdvertRemoveSource(CNode*, unsigned int nChannel, unsigned int nHops, T&) { }

// "Atoms" associated with a key when a block is accepted: no-op.
inline void AddAtomsAndPropagate(uint256, const vector<unsigned short>&, bool) { }

// UI repaint callback (the wxWidgets interface lived in ui.cpp).
void MainFrameRepaint();

#endif // BITCOIN_MARKET_STUB_H
