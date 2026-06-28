// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"

// network-baked (lfnet): use LFnet; keep the original nick
#define IRC_SERVER "irc.lfnet.org"

// irc-fix: IRC server is selectable at build time.
//   default            -> chat.freenode.net  (works on today's Freenode with the \r\n fix below)
//   -DIRC_SERVER='"..."' -> e.g. irc.lfnet.org (LFnet, which still has live Bitcoin nodes)
#ifndef IRC_SERVER
#define IRC_SERVER "chat.freenode.net"
#endif


#pragma pack(1)
struct ircaddr
{
    int ip;
    short port;
};

string EncodeAddress(const CAddress& addr)
{
    struct ircaddr tmp;
    tmp.ip    = addr.ip;
    tmp.port  = addr.port;

    vector<unsigned char> vch(UBEGIN(tmp), UEND(tmp));
    return string("u") + EncodeBase58Check(vch);
}

bool DecodeAddress(string str, CAddress& addr)
{
    vector<unsigned char> vch;
    if (!DecodeBase58Check(str.substr(1), vch))
        return false;

    struct ircaddr tmp;
    if (vch.size() != sizeof(tmp))
        return false;
    memcpy(&tmp, &vch[0], sizeof(tmp));

    addr  = CAddress(tmp.ip, tmp.port);
    return true;
}






static bool Send(SOCKET hSocket, const char* pszSend)
{
    if (strstr(pszSend, "PONG") != pszSend)
        printf("SENDING: %s\n", pszSend);
    const char* psz = pszSend;
    const char* pszEnd = psz + strlen(psz);
    while (psz < pszEnd)
    {
        int ret = send(hSocket, psz, pszEnd - psz, 0);
        if (ret < 0)
            return false;
        psz += ret;
    }
    return true;
}

bool RecvLine(SOCKET hSocket, string& strLine)
{
    strLine = "";
    loop
    {
        char c;
        int nBytes = recv(hSocket, &c, 1, 0);
        if (nBytes > 0)
        {
            if (c == '\n')
                continue;
            if (c == '\r')
                return true;
            strLine += c;
        }
        else if (nBytes <= 0)
        {
            if (!strLine.empty())
                return true;
            // socket closed
            printf("IRC socket closed\n");
            return false;
        }
        else
        {
            // socket error
            int nErr = WSAGetLastError();
            if (nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
            {
                printf("IRC recv failed: %d\n", nErr);
                return false;
            }
        }
    }
}

bool RecvLineIRC(SOCKET hSocket, string& strLine)
{
    loop
    {
        bool fRet = RecvLine(hSocket, strLine);
        if (fRet)
        {
            if (fShutdown)
                return false;
            vector<string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords[0] == "PING")
            {
                strLine[1] = 'O';
                strLine += "\r\n";   // irc-fix: RFC line terminator (modern ircd needs \r\n; LFnet tolerates it)
                Send(hSocket, strLine.c_str());
                continue;
            }
        }
        return fRet;
    }
}

bool RecvUntil(SOCKET hSocket, const char* psz1, const char* psz2=NULL, const char* psz3=NULL)
{
    loop
    {
        string strLine;
        if (!RecvLineIRC(hSocket, strLine))
            return false;
        printf("IRC %s\n", strLine.c_str());
        if (psz1 && strLine.find(psz1) != -1)
            return true;
        if (psz2 && strLine.find(psz2) != -1)
            return true;
        if (psz3 && strLine.find(psz3) != -1)
            return true;
    }
}




bool fRestartIRCSeed = false;

void ThreadIRCSeed(void* parg)
{
    loop
    {
        struct hostent* phostent = gethostbyname(IRC_SERVER);
        CAddress addrConnect(*(u_long*)phostent->h_addr_list[0], htons(6667));

        SOCKET hSocket;
        if (!ConnectSocket(addrConnect, hSocket))
        {
            printf("IRC connect failed\n");
            return;
        }

        if (!RecvUntil(hSocket, "Found your hostname", "using your IP address instead", "Couldn't look up your hostname"))
        {
            closesocket(hSocket);
            return;
        }

#ifdef IRC_FORCE_XNICK
        // Freenode Z-lines any nick that base58-encodes an address: the anti-spam
        // targets the address-blob shape itself (both the 2009 "u..." and a swapped
        // "n..." get banned — the latter registers but is Z-lined seconds after
        // JOIN), not the prefix character. Only an "x<digits>" nick survives, so the
        // Freenode build forces it. That means it cannot advertise a peer there, but
        // it can register, stay connected and mine with a supplied peer.
        string strMyName = strprintf("x%u", GetRand(1000000000));
#else
        string strMyName = EncodeAddress(addrLocalHost);

        if (!addrLocalHost.IsRoutable())
            strMyName = strprintf("x%u", GetRand(1000000000));
#endif

        Send(hSocket, strprintf("NICK %s\r\n", strMyName.c_str()).c_str());
        Send(hSocket, strprintf("USER %s 8 * : %s\r\n", strMyName.c_str(), strMyName.c_str()).c_str());

        if (!RecvUntil(hSocket, " 004 "))
        {
            closesocket(hSocket);
            return;
        }
        Sleep(500);

        Send(hSocket, "JOIN #bitcoin\r\n");
        Send(hSocket, "WHO #bitcoin\r\n");

        while (!fRestartIRCSeed)
        {
            string strLine;
            if (fShutdown || !RecvLineIRC(hSocket, strLine))
            {
                closesocket(hSocket);
                return;
            }
            if (strLine.empty() || strLine[0] != ':')
                continue;
            printf("IRC %s\n", strLine.c_str());

            vector<string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords.size() < 2)
                continue;

            char pszName[10000];
            pszName[0] = '\0';

            if (vWords[1] == "352" && vWords.size() >= 8)
            {
                // index 7 is limited to 16 characters
                // could get full length name at index 10, but would be different from join messages
                strcpy(pszName, vWords[7].c_str());
                printf("GOT WHO: [%s]  ", pszName);
            }

            if (vWords[1] == "JOIN")
            {
                // :username!username@50000007.F000000B.90000002.IP JOIN :#channelname
                strcpy(pszName, vWords[0].c_str() + 1);
                if (strchr(pszName, '!'))
                    *strchr(pszName, '!') = '\0';
                printf("GOT JOIN: [%s]  ", pszName);
            }

            if (pszName[0] == 'u')
            {
                CAddress addr;
                if (DecodeAddress(pszName, addr))
                {
                    CAddrDB addrdb;
                    if (AddAddress(addrdb, addr))
                        printf("new  ");
                    addr.print();
                }
                else
                {
                    printf("decode failed\n");
                }
            }
        }

        fRestartIRCSeed = false;
        closesocket(hSocket);
    }
}










#ifdef TEST
int main(int argc, char *argv[])
{
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2), &wsadata) != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
        return false;
    }

    ThreadIRCSeed(NULL);

    WSACleanup();
    return 0;
}
#endif
