

#include <thread>
#include <unistd.h>

#include "fmt/format.h"

#include "util.hpp"
#include "buffer_helper.hpp"
#include "http_mgr.hpp"
#include "io_msg.hpp"
#include "ws_conn_mgr.hpp"

using std::thread;

void startWsIODataThread(int ioSock) {
    try {
        println("WsIODataThread started");
        while (true) {
            IODataMsg msg = IODataMsg::parse(BufferHelper::readOne(ioSock));
            bool ok = WsConnMgr::sendMsgToTask(msg.taskId, msg.toJsonStr());
            if (!ok)
                printlnTime("WsIOThread, send msg failed, ws probably closed", msg.taskId);
        }
    } catch (json::exception &e) {
        printlnTime("WsIOThread, parse data failed: {}, ignored and continue", e.what());
    } catch (const std::exception &e) {
        printlnTime("WsIOThread, read ioSock failed, {}, shutting down", e.what());
    }
}

void startHttpWsMgr(int cmdSock, int ioSock) {
    thread wsConnThr([]() { WsConnMgr::start(); });
    thread wsIODataThr([ioSock]() { startWsIODataThread(ioSock); });
    HttpConnMgr::start(cmdSock);
    wsConnThr.join();
    wsIODataThr.join();
}