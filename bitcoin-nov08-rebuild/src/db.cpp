// nov08-rebuild - implementation of the rebuilt database layer.
#include "headers.h"


//
// Flat-file backend (replaces Berkeley DB)
//
// Each "table" is a map<bytes,bytes> persisted as a sequence of records:
// [uint32 lenK][K][uint32 lenV][V].
//
static map<string, CDbTable> g_tables;
static set<string>           g_loaded;

static void LoadTableFromDisk(const string& strFile, CDbTable& tbl)
{
    FILE* f = fopen(strFile.c_str(), "rb");
    if (!f)
        return;
    while (true)
    {
        unsigned int lenK = 0, lenV = 0;
        if (fread(&lenK, sizeof(lenK), 1, f) != 1) break;
        vector<unsigned char> k(lenK);
        if (lenK && fread(&k[0], 1, lenK, f) != lenK) break;
        if (fread(&lenV, sizeof(lenV), 1, f) != 1) break;
        vector<unsigned char> v(lenV);
        if (lenV && fread(&v[0], 1, lenV, f) != lenV) break;
        tbl[k] = v;
    }
    fclose(f);
}

CDbTable& GetDbTable(const string& strFile)
{
    CDbTable& tbl = g_tables[strFile];
    if (!g_loaded.count(strFile))
    {
        g_loaded.insert(strFile);
        LoadTableFromDisk(strFile, tbl);
    }
    return tbl;
}

void DbTableFlush(const string& strFile)
{
    map<string, CDbTable>::iterator it = g_tables.find(strFile);
    if (it == g_tables.end())
        return;
    string strTmp = strFile + ".tmp";
    FILE* f = fopen(strTmp.c_str(), "wb");
    if (!f)
        return;
    for (CDbTable::iterator i = it->second.begin(); i != it->second.end(); ++i)
    {
        unsigned int lenK = i->first.size();
        unsigned int lenV = i->second.size();
        fwrite(&lenK, sizeof(lenK), 1, f);
        if (lenK) fwrite(&i->first[0], 1, lenK, f);
        fwrite(&lenV, sizeof(lenV), 1, f);
        if (lenV) fwrite(&i->second[0], 1, lenV, f);
    }
    fclose(f);
    rename(strTmp.c_str(), strFile.c_str());
}

void DBFlush(bool fShutdown)
{
    for (map<string, CDbTable>::iterator it = g_tables.begin(); it != g_tables.end(); ++it)
        DbTableFlush(it->first);
}


//
// CTxDB - transaction index by position (nov08's TxPos scheme)
//
bool CTxDB::ReadTxPos(uint256 hash, CDiskTxPos& pos)
{
    pair<CDiskTxPos, int> value;
    if (!Read(make_pair(string("tx"), hash), value))
        return false;
    pos = value.first;
    return true;
}

bool CTxDB::WriteTxPos(const CTransaction& tx, const CDiskTxPos& pos, int nHeight)
{
    return Write(make_pair(string("tx"), tx.GetHash()), make_pair(pos, nHeight));
}

bool CTxDB::EraseTxPos(const CTransaction& tx)
{
    return Erase(make_pair(string("tx"), tx.GetHash()));
}

bool CTxDB::ContainsTx(uint256 hash)
{
    return Exists(make_pair(string("tx"), hash));
}

bool CTxDB::ReadDiskTx(uint256 hash, CTransaction& tx, FILE** pfileRet)
{
    CDiskTxPos pos;
    if (!ReadTxPos(hash, pos))
        return false;
    return tx.ReadFromDisk(pos, pfileRet);
}

bool CTxDB::ReadOwnerTxes(uint160 hash160, int nHeight, vector<CTransaction>& vtx)
{
    // The owner index lived in the original db.cpp (not released in the nov08
    // preview). Without it we return "not found": a valid outcome that only
    // affects the SPV query for transactions by key.
    return false;
}


//
// CAddrDB - peer addresses
//
bool CAddrDB::WriteAddress(const CAddress& addr)
{
    return Write(make_pair(string("addr"), addr.GetKey()), addr);
}

bool CAddrDB::LoadAddresses()
{
    CRITICAL_BLOCK(cs_mapAddresses)
    {
        for (CDbTable::iterator it = pdb->begin(); it != pdb->end(); ++it)
        {
            try
            {
                CDataStream ssKey((char*)&it->first[0], (char*)&it->first[0] + it->first.size(), SER_DISK);
                string strType;
                ssKey >> strType;
                if (strType == "addr")
                {
                    CDataStream ssValue((char*)&it->second[0], (char*)&it->second[0] + it->second.size(), SER_DISK);
                    CAddress addr;
                    ssValue >> addr;
                    mapAddresses[addr.GetKey()] = addr;
                }
            }
            catch (...) { }
        }
    }
    return true;
}

bool LoadAddresses()
{
    return CAddrDB("cr+").LoadAddresses();
}


//
// CWalletDB - wallet keys and transactions
//
bool CWalletDB::ReadTx(uint256 hash, CWalletTx& wtx)
{
    return Read(make_pair(string("tx"), hash), wtx);
}

bool CWalletDB::WriteTx(uint256 hash, const CWalletTx& wtx)
{
    return Write(make_pair(string("tx"), hash), wtx);
}

bool CWalletDB::EraseTx(uint256 hash)
{
    return Erase(make_pair(string("tx"), hash));
}

bool CWalletDB::LoadWallet()
{
    for (CDbTable::iterator it = pdb->begin(); it != pdb->end(); ++it)
    {
        try
        {
            CDataStream ssKey((char*)&it->first[0], (char*)&it->first[0] + it->first.size(), SER_DISK);
            CDataStream ssValue((char*)&it->second[0], (char*)&it->second[0] + it->second.size(), SER_DISK);
            string strType;
            ssKey >> strType;

            if (strType == "key")
            {
                vector<unsigned char> vchPubKey;
                ssKey >> vchPubKey;
                CPrivKey vchPrivKey;
                ssValue >> vchPrivKey;
                CRITICAL_BLOCK(cs_mapKeys)
                {
                    mapKeys[vchPubKey] = vchPrivKey;
                    mapPubKeys[Hash160(vchPubKey)] = vchPubKey;
                }
            }
            else if (strType == "tx")
            {
                uint256 hash;
                ssKey >> hash;
                CWalletTx wtx;
                ssValue >> wtx;
                CRITICAL_BLOCK(cs_mapWallet)
                    mapWallet[hash] = wtx;
            }
        }
        catch (...) { }
    }
    return true;
}

bool LoadWallet()
{
    return CWalletDB("cr+").LoadWallet();
}
