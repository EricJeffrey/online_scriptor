#include "ws_conn_mgr.hpp"
#include "util.hpp"
#include "config.hpp"

#include "fmt/format.h"

using std::lock_guard;

map<int32_t, WsConnHdl> WsConnMgr::taskId2Handle;
map<WsConnHdl, int32_t, std::owner_less<WsConnHdl>> WsConnMgr::handle2TaskId;
mutex WsConnMgr::handleLock;
WsServer WsConnMgr::server;

void WsConnMgr::addHandle(int32_t taskId, WsConnHdl hdl) {
    lock_guard<mutex> guard(handleLock);
    taskId2Handle[taskId] = hdl;
    handle2TaskId[hdl] = taskId;
}
void WsConnMgr::removeHandle(int32_t taskId) {
    lock_guard<mutex> guard(handleLock);
    if (taskId2Handle.find(taskId) != taskId2Handle.end()) {
        handle2TaskId.erase(taskId2Handle[taskId]);
        taskId2Handle.erase(taskId);
    }
}
void WsConnMgr::removeHandle(WsConnHdl hdl) {
    lock_guard<mutex> guard(handleLock);
    if (handle2TaskId.find(hdl) != handle2TaskId.end()) {
        taskId2Handle.erase(handle2TaskId[hdl]);
        handle2TaskId.erase(hdl);
    }
}
bool WsConnMgr::sendMsgToTask(int32_t taskId, const string &msg) {
    lock_guard<mutex> guard(handleLock);
    if (taskId2Handle.find(taskId) != taskId2Handle.end()) {
        try {
            server.send(taskId2Handle[taskId], msg, websocketpp::frame::opcode::TEXT);
            return true;
        } catch (const std::exception &e) {
            printlnTime("WsConnMgr.sendMsgToTask failed, websocket probably closed");
        }
    }
    return false;
}

inline int32_t extractTaskId(const string &path) {
    if (auto idx = path.find("?taskId="); idx != string::npos) {
        try {
            int32_t taskId = std::stoi(path.substr(idx + 8));
            if (taskId > 0)
                return taskId;
        } catch (const std::exception &e) {
            printlnTime("WsConnMgr: extractTaskId from path failed: {}", e.what());
        }
    }
    return -1;
}

void WsConnMgr::start() {
    WsConnMgr::server.set_access_channels(websocketpp::log::alevel::fail);
    WsConnMgr::server.clear_access_channels(websocketpp::log::alevel::frame_header);
    WsConnMgr::server.clear_access_channels(websocketpp::log::alevel::frame_payload);
    WsConnMgr::server.clear_access_channels(websocketpp::log::alevel::control);
    WsConnMgr::server.clear_access_channels(websocketpp::log::alevel::connect);
    WsConnMgr::server.init_asio();

    WsConnMgr::server.set_open_handler([](WsConnHdl hdl) {
        const string &path = WsConnMgr::server.get_con_from_hdl(hdl)->get_request().get_uri();
        int32_t taskId = extractTaskId(path);
        if (taskId == -1) {
            WsConnMgr::server.close(hdl, websocketpp::close::status::invalid_subprotocol_data,
                                    "taskId is required");
        } else {
            // TODO time + msg
            printlnTime("Websocket established, taskId: {}", taskId);
            WsConnMgr::addHandle(taskId, hdl);
        }
    });
    WsConnMgr::server.set_close_handler([](WsConnHdl hdl) { WsConnMgr::removeHandle(hdl); });
    WsConnMgr::server.set_message_handler([](WsConnHdl hdl, auto msgPtr) {
        printlnTime("Websocket got message from client, ignored");
    });
    WsConnMgr::server.listen(PORT_WS);
    WsConnMgr::server.start_accept();
    println("Websocket server started on port: {}", PORT_WS);
    WsConnMgr::server.run();
    printlnTime("Websocket server exited");
}
