#if !defined(HTTP_MGR)
#define HTTP_MGR

#include "uWebSockets/App.h"

#include <cstdlib>
#include <map>

using std::map;

struct PerWsData {
    int32_t taskId;
    uWS::WebSocket<false, true, PerWsData> *ws;
};

struct HttpMgr {
    static int mIOSock;
    static int mCmdSock;
    static map<int32_t, PerWsData *> taskId2WsData;

    static void start(int cmdSock, int ioSock);
};

#endif // HTTP_MGR
