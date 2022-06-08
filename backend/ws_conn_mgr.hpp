#if !defined(WS_CONN_MGR)
#define WS_CONN_MGR

#include "websocketpp/config/asio.hpp"
#include "websocketpp/server.hpp"

#include <map>
#include <mutex>
#include <string>

using WsServer = websocketpp::server<websocketpp::config::asio>;
using WsConnHdl = websocketpp::connection_hdl;
using std::map, std::string, std::mutex;

struct WsConnMgr {
    static WsServer server;
    static map<int32_t, WsConnHdl> taskId2Handle;
    static map<WsConnHdl, int32_t, std::owner_less<WsConnHdl>> handle2TaskId;
    static mutex handleLock;

    static void start();

    static void addHandle(int32_t taskId, WsConnHdl hdl);
    static void removeHandle(int32_t taskId);
    static void removeHandle(WsConnHdl hdl);
    static bool sendMsgToTask(int32_t taskId, const string &msg);
};

#endif // WS_CONN_MGR
