#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/thread.h"
#include "event2/util.h"

#include "cmd_mgr.hpp"
#include "io_mgr.hpp"
#include <thread>

int CmdMgr::cmdSock = -1;
event_base *CmdMgr::base = nullptr;
RunningTaskHelper CmdMgr::runningTaskHelper;

void startTaskMgr(int unixSock, int ioSock) {
    if (evthread_use_pthreads() == -1)
        throw runtime_error("call to evthread_use_pthread failed!");
    auto ioMgrThread = std::thread([](int ioSock) { IOMgr::start(ioSock); }, ioSock);
    CmdMgr::start(unixSock);
    printf("Stopping IOMgr\n");
    event_base_loopexit(IOMgr::base, nullptr);
    ioMgrThread.join();
    printf("CmdMgr & IOMgr exited\n");
}

void CmdMgr::start(int unixSock) {
    evutil_make_socket_closeonexec(unixSock);
}