// nov08-rebuild - definitions for the market stubs (see market.h).
#include "headers.h"

map<uint256, CProduct> mapProducts;
map<uint256, CTable>   mapTables;
CCriticalSection cs_mapProducts;
CCriticalSection cs_mapTables;

// Methods declared in nov08's headers whose body lived in the unreleased part.
// Provided with minimal, safe behavior.

// Lightweight client (SPV) mode: only invoked if fClient==true, which we don't use.
bool CTransaction::ClientConnectInputs()
{
    return false;
}

// Cancel the subscription to a broadcast channel (market subsystem, inert).
void CNode::CancelSubscribe(unsigned int nChannel)
{
    if (nChannel < vfSubscribe.size())
        vfSubscribe[nChannel] = false;
}
