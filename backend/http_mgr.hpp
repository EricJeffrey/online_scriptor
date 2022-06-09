#if !defined(HTTP_MGR)
#define HTTP_MGR

#include <mutex>

void startHttpWsMgr(int cmdSock, int ioSock);

void startWsIODataThread(int ioSock);

struct HttpConnMgr {
    static int cmdSock;
    static std::mutex sockMutex;

    static void start(int cmdSock);
};

#endif // HTTP_MGR
