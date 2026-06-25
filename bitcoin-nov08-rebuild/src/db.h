// nov08-rebuild - rebuilt database layer.
//
// The November 2008 preview did not include db.h/db.cpp. Here we rebuild the
// interface that Satoshi's code actually uses (deduced from the call sites in
// main.cpp/node.cpp): an index by transaction POSITION ("TxPos"), not the later
// CTxIndex of 0.1.0.
//
// Backend: instead of Berkeley DB we use an in-memory key/value store
// (map<bytes,bytes>) persisted to a flat file per table. It keeps the same
// serialization semantics (CDataStream) as the original.
#ifndef BITCOIN_DB_H
#define BITCOIN_DB_H

class CTransaction;
class CDiskTxPos;
class CAddress;
class CWalletTx;

extern bool fClient;  // defined in node.cpp

// In-memory table (serialized key -> serialized value), persisted to disk.
typedef map<vector<unsigned char>, vector<unsigned char> > CDbTable;
CDbTable& GetDbTable(const string& strFile);   // db.cpp
void      DbTableFlush(const string& strFile); // db.cpp
void      DBFlush(bool fShutdown);             // db.cpp (flushes every table)


class CDB
{
protected:
    string strFile;
    CDbTable* pdb;

    explicit CDB(const char* pszFile, const char* pszMode="r+")
    {
        strFile = (pszFile ? pszFile : "");
        pdb = (pszFile ? &GetDbTable(strFile) : NULL);
    }
    ~CDB() { }

private:
    CDB(const CDB&);
    void operator=(const CDB&);

    template<typename K>
    vector<unsigned char> SerializeKey(const K& key)
    {
        CDataStream ssKey(SER_DISK);
        ssKey.reserve(1000);
        ssKey << key;
        return vector<unsigned char>((unsigned char*)&ssKey[0],
                                     (unsigned char*)&ssKey[0] + ssKey.size());
    }

protected:
    template<typename K, typename T>
    bool Read(const K& key, T& value)
    {
        if (!pdb) return false;
        CDbTable::iterator it = pdb->find(SerializeKey(key));
        if (it == pdb->end() || it->second.empty())
            return false;
        try
        {
            CDataStream ssValue((char*)&it->second[0],
                                (char*)&it->second[0] + it->second.size(), SER_DISK);
            ssValue >> value;
        }
        catch (...) { return false; }
        return true;
    }

    template<typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite=true)
    {
        if (!pdb) return false;
        vector<unsigned char> vKey = SerializeKey(key);
        if (!fOverwrite && pdb->count(vKey))
            return false;
        CDataStream ssValue(SER_DISK);
        ssValue.reserve(10000);
        ssValue << value;
        (*pdb)[vKey] = vector<unsigned char>((unsigned char*)&ssValue[0],
                                             (unsigned char*)&ssValue[0] + ssValue.size());
        DbTableFlush(strFile);
        return true;
    }

    template<typename K>
    bool Erase(const K& key)
    {
        if (!pdb) return false;
        pdb->erase(SerializeKey(key));
        DbTableFlush(strFile);
        return true;
    }

    template<typename K>
    bool Exists(const K& key)
    {
        if (!pdb) return false;
        return pdb->count(SerializeKey(key)) > 0;
    }

public:
    // No real transactions: the store is flushed after each write.
    bool TxnBegin()  { return true; }
    bool TxnCommit() { return true; }
    bool TxnAbort()  { return true; }
    bool ReadVersion(int& nVersion) { nVersion = 0; return Read(string("version"), nVersion); }
    bool WriteVersion(int nVersion) { return Write(string("version"), nVersion); }
    void Close() { }
};


// Transaction index by on-disk position (blkindex.dat)
class CTxDB : public CDB
{
public:
    CTxDB(const char* pszMode="r+") : CDB(!fClient ? "blkindex.dat" : NULL, pszMode) { }
private:
    CTxDB(const CTxDB&);
    void operator=(const CTxDB&);
public:
    bool ReadTxPos(uint256 hash, CDiskTxPos& pos);
    bool WriteTxPos(const CTransaction& tx, const CDiskTxPos& pos, int nHeight);
    bool EraseTxPos(const CTransaction& tx);
    bool ContainsTx(uint256 hash);
    bool ReadDiskTx(uint256 hash, CTransaction& tx, FILE** pfileRet=NULL);
    bool ReadOwnerTxes(uint160 hash160, int nHeight, vector<CTransaction>& vtx);
};


// Known peer addresses (addr.dat)
class CAddrDB : public CDB
{
public:
    CAddrDB(const char* pszMode="r+") : CDB("addr.dat", pszMode) { }
private:
    CAddrDB(const CAddrDB&);
    void operator=(const CAddrDB&);
public:
    bool WriteAddress(const CAddress& addr);
    bool LoadAddresses();
};
bool LoadAddresses();


// Wallet: own keys and transactions (wallet.dat)
class CWalletDB : public CDB
{
public:
    CWalletDB(const char* pszMode="r+") : CDB("wallet.dat", pszMode) { }
private:
    CWalletDB(const CWalletDB&);
    void operator=(const CWalletDB&);
public:
    bool ReadTx(uint256 hash, CWalletTx& wtx);
    bool WriteTx(uint256 hash, const CWalletTx& wtx);
    bool EraseTx(uint256 hash);

    bool WriteKey(const vector<unsigned char>& vchPubKey, const CPrivKey& vchPrivKey)
    {
        return Write(make_pair(string("key"), vchPubKey), vchPrivKey, false);
    }
    bool ReadDefaultKey(vector<unsigned char>& vchPubKey)
    {
        vchPubKey.clear();
        return Read(string("defaultkey"), vchPubKey);
    }
    bool WriteDefaultKey(const vector<unsigned char>& vchPubKey)
    {
        return Write(string("defaultkey"), vchPubKey);
    }
    bool LoadWallet();
};
bool LoadWallet();

#endif // BITCOIN_DB_H
