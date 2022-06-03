#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/thread.h"
#include "event2/util.h"

#include "cmd_mgr.hpp"
#include "cmd_msg.hpp"
#include "io_mgr.hpp"
#include "script_helper.hpp"
#include "task_db_helper.hpp"

#include <csignal>
#include <fstream>
#include <ostream>
#include <thread>

#include <sys/wait.h>
#include <unistd.h>

int CmdMgr::cmdSock = -1;
event_base *CmdMgr::base = nullptr;
event *CmdMgr::evChild = nullptr;
bufferevent *CmdMgr::bevCmdSock = nullptr;
RunningTaskHelper CmdMgr::runningTaskHelper;
BufferHelper CmdMgr::bufferHelper;

constexpr char SCRIPT_ROOT_PATH[] = "/data/online_scriptor/";

void startTaskMgr(int cmdSock, int ioSock) {
    if (evthread_use_pthreads() == -1)
        throw runtime_error("call to evthread_use_pthread failed!");
    auto ioMgrThread = std::thread([ioSock]() { IOMgr::start(ioSock); });
    CmdMgr::start(cmdSock);
    printf("CmdMgr exited, stopping IOMgr\n");
    event_base_loopexit(IOMgr::base, nullptr);
    ioMgrThread.join();
    printf("CmdMgr & IOMgr exited\n");
}

void startTask(const Task &task) {
    const string filePath = SCRIPT_ROOT_PATH + std::to_string(task.id);
    std::ofstream ofs(filePath);
    ofs.write(task.scriptCode.c_str(), task.scriptCode.size());
    ofs.close();

    // 需要关闭其它进程的IO重定向 fd
    vector<int> fdsToClose = CmdMgr::runningTaskHelper.getFdsExcept(task.id);
    StartScriptRes scriptRes;
    if (task.scriptType == TaskScriptType::PYTHON)
        scriptRes = ScriptHelper::startPyScriptWithIORedir(filePath, fdsToClose);
    if (task.scriptType == TaskScriptType::BASH)
        scriptRes = ScriptHelper::startBashScriptWithIORedir(filePath, fdsToClose);

    IOMgr::addFds(task.id,
                  {scriptRes.fdStdin, scriptRes.fdStdout, scriptRes.fdStderr});
    CmdMgr::runningTaskHelper.add(task.id, scriptRes.childPid,
                                  scriptRes.fdStdin, scriptRes.fdStdout, scriptRes.fdStderr);
    Task newTask = task;
    newTask.pid = scriptRes.childPid;
    newTask.timesExecuted += 1;
    newTask.status = TaskStatus::RUNNING;
    TaskDBHelper::updateTask(task.id, newTask);
}

void handleCmdMsg(const CmdMsg &msg) {
    // TODO dispatch
    switch (msg.cmdType) {
    case CmdType::CREATE_TASK:
        printf("Ok I Will CREATE_TASK\n");
        break;
    case CmdType::START_TASK:
        printf("Ok I Will START_TASK\n");
        break;
    case CmdType::STOP_TASK:
        printf("Ok I Will STOP_TASK\n");
        break;
    case CmdType::DELETE_TASK:
        printf("Ok I Will DELETE_TASK\n");
        break;
    case CmdType::GET_TASK:
        printf("Ok I Will GET_TASK\n");
        break;
    case CmdType::ENABLE_REDIRECT:
        printf("Ok I Will ENABLE_REDIRECT\n");
        break;
    case CmdType::DISABLE_REDIRECT:
        printf("Ok I Will DISABLE_REDIRECT\n");
        break;
    case CmdType::PUT_TO_STDIN:
        printf("Ok I Will PUT_TO_STDIN\n");
        break;
    case CmdType::SHUT_DOWN:
        printf("Ok I Will SHUT_DOWN\n");
        break;
    }
}

void onCmdSockReadCb(bufferevent *bev, void *arg) {
    char data[BUFSIZ];
    size_t n = 0;
    while ((n = bufferevent_read(bev, data, BUFSIZ)) > 0) {
        auto jsonList = CmdMgr::bufferHelper.tryFullFill(data, n);
        for (auto &&jsonStr : jsonList)
            handleCmdMsg(CmdMsg::parse(jsonStr));
    }
}

void onCmdSockEventCb(bufferevent *bev, short events, void *arg) {
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        printf("CmdMgr: ERROR or EOF on cmdSock, stopping\n");
        bufferevent_free(bev);
        event_base_loopexit(CmdMgr::base, nullptr);
    }
}

void onSIGCHILDCb(evutil_socket_t fd, short events, void *arg) {
    if (events & EV_SIGNAL) {
        // wait child and remove their file descriptors
        pid_t pid = 0;
        int status = 0;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            auto fds = CmdMgr::runningTaskHelper.getFdsByPid(pid);
            IOMgr::removeFds(fds);
        }
    }
}

void CmdMgr::start(int sock) {
    evutil_make_socket_closeonexec(sock);
    CmdMgr::cmdSock = cmdSock;
    CmdMgr::base = event_base_new();
    assert(base != nullptr);

    // read on cmdSock
    CmdMgr::bevCmdSock = bufferevent_socket_new(CmdMgr::base, CmdMgr::cmdSock, BEV_OPT_CLOSE_ON_FREE);
    assert(CmdMgr::bevCmdSock != nullptr);
    bufferevent_setcb(CmdMgr::bevCmdSock, onCmdSockReadCb, nullptr, onCmdSockEventCb, CmdMgr::base);
    int res = bufferevent_enable(CmdMgr::bevCmdSock, EV_READ | EV_WRITE);
    assert(res != -1);

    // SIGCHILD
    CmdMgr::evChild = evsignal_new(CmdMgr::base, SIGCHLD, onSIGCHILDCb, CmdMgr::base);
    assert(CmdMgr::evChild != nullptr);
    res = evsignal_add(CmdMgr::evChild, nullptr);
    assert(res != -1);

    res = event_base_loop(CmdMgr::base, EVLOOP_NO_EXIT_ON_EMPTY);
    assert(res != -1);
}

void CmdMgr::stop() {
    event_free(CmdMgr::evChild);
    bufferevent_free(CmdMgr::bevCmdSock);
    event_base_loopexit(CmdMgr::base, nullptr);
    event_base_free(CmdMgr::base);
}
