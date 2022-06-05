#include <unistd.h>

#include "fmt/format.h"

#include "buffer_helper.hpp"
#include "http_mgr.hpp"
#include "io_msg.hpp"

void wsIODataThread(int ioSock) {
    try {
        while (true) {
            IODataMsg msg = IODataMsg::parse(BufferHelper::readOne(ioSock));
            bool ok = HttpMgr::taskWsHelper.sendMsgOnTaskWs(msg.taskId, msg.toJsonStr());
            if (!ok)
                fmt::print("WsIOThread, send msg failed, ws probably closed\n", msg.taskId);
        }
    } catch (json::exception &e) {
        fmt::print("WsIOThread, parse data failed: {}, ignored and continue\n", e.what());
    } catch (const std::exception &e) {
        fmt::print("WsIOThread, read ioSock failed, {}, shutting down\n", e.what());
    }
}
