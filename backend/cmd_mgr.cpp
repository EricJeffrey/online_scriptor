#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"

#include "cmd_mgr.hpp"
#include "cmd_msg.hpp"
#include "io_mgr.hpp"
#include "script_helper.hpp"
#include "task_db_helper.hpp"

#include <csignal>
#include <fstream>
#include <ostream>

#include <sys/wait.h>
#include <unistd.h>

int CmdMgr::cmdSock = -1;
event_base *CmdMgr::base = nullptr;
event *CmdMgr::evChild = nullptr;
bufferevent *CmdMgr::bevCmdSock = nullptr;
RunningTaskHelper CmdMgr::runningTaskHelper;
BufferHelper CmdMgr::bufferHelper;

constexpr char SCRIPT_ROOT_PATH[] = "/data/online_scriptor/";

CmdRes createTask(const CmdMsg &msg) {
    // printf("__DEBUG cmd_mgr:createTask\n");
    int32_t newTaskId = TaskDBHelper::createTask(msg.title, msg.scriptCode, msg.scriptType,
                                                 msg.interval, msg.maxTimes);
    return CmdRes{.status = CmdResType::OK, .taskId = newTaskId};
}

CmdRes startTask(const CmdMsg &msg) {
    if (!TaskDBHelper::hasTask(msg.taskId))
        return CmdRes{.status = CmdResType::NO_SUCH_TASK};
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId))
        return CmdRes{.status = CmdResType::TASK_IS_RUNNING};
    Task task = TaskDBHelper::getTask(msg.taskId);
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

    IOMgr::addFds(task.id, {scriptRes.fdStdin, scriptRes.fdStdout, scriptRes.fdStderr});
    CmdMgr::runningTaskHelper.add(task.id, scriptRes.childPid, scriptRes.fdStdin,
                                  scriptRes.fdStdout, scriptRes.fdStderr);
    Task newTask = task;
    newTask.pid = scriptRes.childPid;
    newTask.timesExecuted += 1;
    newTask.status = TaskStatus::RUNNING;
    TaskDBHelper::updateTask(task.id, newTask);

    return CmdRes{.status = CmdResType::OK};
}

CmdRes stopTask(const CmdMsg &msg) {
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        kill(CmdMgr::runningTaskHelper.taskId2RunningTask[msg.taskId]->mPid, SIGKILL);
        return CmdRes{.status = CmdResType::OK};
    }
    return CmdRes{.status = CmdResType::TASK_NOT_RUNNING};
}

CmdRes deleteTask(const CmdMsg &msg) {
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        stopTask(msg);
    }
    if (TaskDBHelper::hasTask(msg.taskId)) {
        TaskDBHelper::deleteTask(msg.taskId);
        return CmdRes{.status = CmdResType::OK};
    } else {
        return CmdRes{.status = CmdResType::NO_SUCH_TASK};
    }
}

CmdRes enableIORedirect(const CmdMsg &msg) {
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        auto fds = CmdMgr::runningTaskHelper.getFdsByTaskId(msg.taskId);
        IOMgr::enableRedirect({fds[1], fds[2]});
        return CmdRes{.status = CmdResType::OK};
    }
    return CmdRes{.status = CmdResType::TASK_NOT_RUNNING};
}

CmdRes disableIORedirect(const CmdMsg &msg) {
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        auto fds = CmdMgr::runningTaskHelper.getFdsByTaskId(msg.taskId);
        IOMgr::disableRedirect({fds[1], fds[2]});
        return CmdRes{.status = CmdResType::OK};
    }
    return CmdRes{.status = CmdResType::TASK_NOT_RUNNING};
}

CmdRes putToTaskInput(const CmdMsg &msg) {
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        int fdIn = CmdMgr::runningTaskHelper.getFdsByTaskId(msg.taskId).at(0);
        IOMgr::putToStdin(fdIn, msg.stdinContent);
        return CmdRes{.status = CmdResType::OK};
    }
    return CmdRes{.status = CmdResType::TASK_NOT_RUNNING};
}

CmdRes getTask(const CmdMsg &msg) {
    if (TaskDBHelper::hasTask(msg.taskId)) {
        return CmdRes{
            .status = CmdResType::OK,
            .task = TaskDBHelper::getTask(msg.taskId),
        };
    }
    return CmdRes{.status = CmdResType::NO_SUCH_TASK};
}

CmdRes getAllTask(const CmdMsg &msg) {
    return CmdRes{
        .status = CmdResType::OK,
        .taskList = TaskDBHelper::getAllTask(),
    };
}

void writeBackCmdRes(const CmdRes &cmdRes) {
    // printf("__DEBUG cmd_mgr: writebackcmdres, data: %s\n", cmdRes.toJsonStr().c_str());
    string data = BufferHelper::make(cmdRes.toJsonStr());
    int res = bufferevent_write(CmdMgr::bevCmdSock, data.data(), data.size());
    assert(res != -1);
}

void handleCmdMsg(const CmdMsg &msg) {
    try {
        CmdRes resMsg;
        switch (msg.cmdType) {
        case CmdType::CREATE_TASK:
            printf("Ok I Will CREATE_TASK, msg: %s\n", msg.toJsonStr().c_str());
            resMsg = createTask(msg);
            break;
        case CmdType::START_TASK:
            printf("Ok I Will START_TASK, msg: %s\n", msg.toJsonStr().c_str());
            resMsg = startTask(msg);
            break;
        case CmdType::STOP_TASK:
            printf("Ok I Will STOP_TASK, msg: %s\n", msg.toJsonStr().c_str());
            resMsg = stopTask(msg);
            break;
        case CmdType::DELETE_TASK:
            printf("Ok I Will DELETE_TASK, msg: %s\n", msg.toJsonStr().c_str());
            resMsg = deleteTask(msg);
            break;
        case CmdType::GET_TASK:
            printf("Ok I Will GET_TASK, msg: %s\n", msg.toJsonStr().c_str());
            resMsg = getTask(msg);
            break;
        case CmdType::GET_ALL_TASK:
            printf("Ok I Will GET_ALL_TASK, msg: %s\n", msg.toJsonStr().c_str());
            resMsg = getAllTask(msg);
            break;
        case CmdType::ENABLE_REDIRECT:
            printf("Ok I Will ENABLE_REDIRECT, msg: %s\n", msg.toJsonStr().c_str());
            resMsg = enableIORedirect(msg);
            break;
        case CmdType::DISABLE_REDIRECT:
            printf("Ok I Will DISABLE_REDIRECT, msg: %s\n", msg.toJsonStr().c_str());
            resMsg = disableIORedirect(msg);
            break;
        case CmdType::PUT_TO_STDIN:
            printf("Ok I Will PUT_TO_STDIN, msg: %s\n", msg.toJsonStr().c_str());
            resMsg = putToTaskInput(msg);
            break;
        case CmdType::SHUT_DOWN:
            printf("Ok I Will SHUT_DOWN, msg: %s\n", msg.toJsonStr().c_str());
            CmdMgr::stop();
            resMsg.status = CmdResType::OK;
            break;
        default:
            printf("handle cmdmsg: invalid msg type, ignored\n");
            resMsg.status = CmdResType::INVALID_CMD_TYPE;
            break;
        }
        writeBackCmdRes(resMsg);
    } catch (const std::exception &e) {
        fprintf(stderr, "Handle CmdMsg failed, %s\n", e.what());
        writeBackCmdRes(CmdRes{.status = CmdResType::FAILED});
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
        printf("CmdMgr: ERROR or EOF on cmdSock, shutting down CmdMgr\n");
        bufferevent_free(bev);
        event_base_loopexit(CmdMgr::base, nullptr);
    }
}

void onSIGCHILDCb(evutil_socket_t fd, short events, void *arg) {
    time_t timestamp = time(nullptr);
    if (events & EV_SIGNAL) {
        // wait child and remove their file descriptors
        pid_t pid = 0;
        int status = 0;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            auto fds = CmdMgr::runningTaskHelper.getFdsByPid(pid);
            IOMgr::removeFds(fds);
            CmdMgr::runningTaskHelper.removeByPid(pid);
            int32_t taskId = CmdMgr::runningTaskHelper.pid2RunningTask[pid]->mTaskId;
            Task newTask = TaskDBHelper::getTask(taskId);
            newTask.exitCode = WEXITSTATUS(status);
            newTask.exitTimeStamp = timestamp;
            newTask.status = TaskStatus::IDLE;
            newTask.pid = -1;
            TaskDBHelper::updateTask(taskId, newTask);
            // TODO 设定间隔 task.interval 之后的timer
        }
    }
}

void CmdMgr::start(int sock) {
    evutil_make_socket_closeonexec(sock);
    CmdMgr::cmdSock = sock;
    CmdMgr::base = event_base_new();
    assert(base != nullptr);

    // read on cmdSock
    CmdMgr::bevCmdSock =
        bufferevent_socket_new(CmdMgr::base, CmdMgr::cmdSock, BEV_OPT_CLOSE_ON_FREE);
    assert(CmdMgr::bevCmdSock != nullptr);
    bufferevent_setcb(CmdMgr::bevCmdSock, onCmdSockReadCb, nullptr, onCmdSockEventCb, CmdMgr::base);
    int res = bufferevent_enable(CmdMgr::bevCmdSock, EV_READ | EV_WRITE);
    assert(res != -1);

    // SIGCHILD
    CmdMgr::evChild = evsignal_new(CmdMgr::base, SIGCHLD, onSIGCHILDCb, CmdMgr::base);
    assert(CmdMgr::evChild != nullptr);
    res = evsignal_add(CmdMgr::evChild, nullptr);
    assert(res != -1);

    // write 1 byte to let the parent know we are ready to go
    {
        while (true) {
            // printf("__DEBUG cmd_mgr writing notification\n");
            int res = write(CmdMgr::cmdSock, "1", 1);
            if (res == 1)
                break;
            if (res == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
                continue;
            else
                throw runtime_error("CmdMgr, write notification to CmdSock failed! " +
                                    string(strerror(errno)));
        }
    }

    res = event_base_loop(CmdMgr::base, EVLOOP_NO_EXIT_ON_EMPTY);
    assert(res != -1);
}

void CmdMgr::stop() {
    event_free(CmdMgr::evChild);
    bufferevent_free(CmdMgr::bevCmdSock);
    event_base_loopexit(CmdMgr::base, nullptr);
    event_base_free(CmdMgr::base);
}
