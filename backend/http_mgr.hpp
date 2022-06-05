#if !defined(HTTP_MGR)
#define HTTP_MGR

#include "uWebSockets/App.h"

#include <cstdlib>
#include <map>
#include <mutex>
#include <string>

using std::map, std::mutex, std::lock_guard;
using std::string;

struct PerWsData {
    int32_t taskId;
    uWS::WebSocket<false, true, PerWsData> *ws;
};

struct TaskWsHelper {
    map<int32_t, PerWsData *> taskId2WsData;
    mutex lock;

    void add(int32_t taskId, PerWsData *data) {
        lock_guard<mutex> guard(lock);
        taskId2WsData[taskId] = data;
    }
    void remove(int32_t taskId) {
        lock_guard<mutex> guard(lock);
        taskId2WsData.erase(taskId);
    }

    bool has(int32_t taskId) {
        lock_guard<mutex> guard(lock);
        return taskId2WsData.find(taskId) != taskId2WsData.end();
    }

    /**
     * @brief 使用taskId对应的websocket连接发送msg
     *
     * @param taskId taskId
     * @param msg 消息
     * @return true 发送成功，false 任务的Ws不存在或发生异常
     */
    bool sendMsgOnTaskWs(int32_t taskId, const string &msg);
};

struct HttpMgr {
    static int mIOSock;
    static int mCmdSock;
    static TaskWsHelper taskWsHelper;

    static void start(int cmdSock, int ioSock);
};

void wsIODataThread(int);

#endif // HTTP_MGR
