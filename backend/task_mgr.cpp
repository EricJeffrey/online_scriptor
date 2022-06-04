#include "task_mgr.hpp"
#include "cmd_mgr.hpp"
#include "io_mgr.hpp"
#include "task_db_helper.hpp"

#include "event2/thread.h"
#include "fmt/format.h"
#include <thread>


void startTaskMgr(int cmdSock, int ioSock) {
    if (evthread_use_pthreads() == -1)
        throw runtime_error("call to evthread_use_pthread failed!");
    TaskDBHelper::init();
    fmt::print("starting IOMgr\n");
    auto ioMgrThread = std::thread([ioSock]() { IOMgr::start(ioSock); });
    fmt::print("starting CmdMgr\n");
    CmdMgr::start(cmdSock);
    fmt::print("CmdMgr exited, waiting IOMgr\n");
    ioMgrThread.join();
    fmt::print("CmdMgr & IOMgr exited\n");
}