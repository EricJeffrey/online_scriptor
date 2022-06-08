#include "task_mgr.hpp"
#include "cmd_mgr.hpp"
#include "io_mgr.hpp"
#include "task_db_helper.hpp"
#include "util.hpp"

#include "event2/thread.h"
#include "fmt/format.h"
#include <thread>


void startTaskMgr(int cmdSock, int ioSock) {
    if (evthread_use_pthreads() == -1)
        throw runtime_error("call to evthread_use_pthread failed!");
    TaskDBHelper::init();
    println("TaskMgr: starting IOMgr");
    auto ioMgrThread = std::thread([ioSock]() { IOMgr::start(ioSock); });
    println("TaskMgr: starting CmdMgr");
    CmdMgr::start(cmdSock);
    println("TaskMgr: CmdMgr exited, waiting IOMgr");
    ioMgrThread.join();
    println("TaskMgr: CmdMgr & IOMgr exited");
}