#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"

#include "cmd_mgr.hpp"
#include "cmd_msg.hpp"
#include "config.hpp"
#include "io_mgr.hpp"
#include "script_helper.hpp"
#include "task_db_helper.hpp"

#include "fmt/core.h"

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

CmdRes createTask(const CmdMsg &msg) {
    int32_t newTaskId = TaskDBHelper::createTask(msg.title, msg.scriptCode, msg.scriptType,
                                                 msg.interval, msg.maxTimes);
    return CmdRes{.status = CmdRes::Type::OK, .taskId = newTaskId};
}

CmdRes startTask(const CmdMsg &msg) {
    if (!TaskDBHelper::hasTask(msg.taskId))
        return CmdRes{.status = CmdRes::Type::NO_SUCH_TASK};
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId))
        return CmdRes{.status = CmdRes::Type::TASK_IS_RUNNING};
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

    return CmdRes{.status = CmdRes::Type::OK};
}

CmdRes stopTask(const CmdMsg &msg) {
    if (!TaskDBHelper::hasTask(msg.taskId))
        return CmdRes{.status = CmdRes::Type::NO_SUCH_TASK};
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        kill(CmdMgr::runningTaskHelper.taskId2RunningTask.at(msg.taskId)->mPid, SIGKILL);
        return CmdRes{.status = CmdRes::Type::OK};
    }
    return CmdRes{.status = CmdRes::Type::TASK_NOT_RUNNING};
}

CmdRes deleteTask(const CmdMsg &msg) {
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        stopTask(msg);
    }
    if (TaskDBHelper::hasTask(msg.taskId)) {
        TaskDBHelper::deleteTask(msg.taskId);
        return CmdRes{.status = CmdRes::Type::OK};
    } else {
        return CmdRes{.status = CmdRes::Type::NO_SUCH_TASK};
    }
}

CmdRes enableIORedirect(const CmdMsg &msg) {
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        auto fds = CmdMgr::runningTaskHelper.getFdsByTaskId(msg.taskId);
        IOMgr::enableRedirect({fds[1], fds[2]});
        return CmdRes{.status = CmdRes::Type::OK};
    }
    return CmdRes{.status = CmdRes::Type::TASK_NOT_RUNNING};
}

CmdRes disableIORedirect(const CmdMsg &msg) {
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        auto fds = CmdMgr::runningTaskHelper.getFdsByTaskId(msg.taskId);
        IOMgr::disableRedirect({fds[1], fds[2]});
        return CmdRes{.status = CmdRes::Type::OK};
    }
    return CmdRes{.status = CmdRes::Type::TASK_NOT_RUNNING};
}

CmdRes putToTaskInput(const CmdMsg &msg) {
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId)) {
        int fdIn = CmdMgr::runningTaskHelper.getFdsByTaskId(msg.taskId).at(0);
        IOMgr::putToStdin(fdIn, msg.stdinContent);
        return CmdRes{.status = CmdRes::Type::OK};
    }
    return CmdRes{.status = CmdRes::Type::TASK_NOT_RUNNING};
}

CmdRes getTask(const CmdMsg &msg) {
    if (TaskDBHelper::hasTask(msg.taskId)) {
        return CmdRes{
            .status = CmdRes::Type::OK,
            .task = TaskDBHelper::getTask(msg.taskId),
        };
    }
    return CmdRes{.status = CmdRes::Type::NO_SUCH_TASK};
}

CmdRes getAllTask(const CmdMsg &msg) {
    return CmdRes{
        .status = CmdRes::Type::OK,
        .taskList = TaskDBHelper::getAllTask(),
    };
}

void writeBackCmdRes(const CmdRes &cmdRes) {
    string data = BufferHelper::make(cmdRes.toJsonStr());
    int res = bufferevent_write(CmdMgr::bevCmdSock, data.data(), data.size());
    assert(res != -1);
}

void handleCmdMsg(const CmdMsg &msg) {
    try {
        CmdRes resMsg;
        switch (msg.cmdType) {
        case CmdMsg::Type::CREATE_TASK:
            resMsg = createTask(msg);
            break;
        case CmdMsg::Type::START_TASK:
            resMsg = startTask(msg);
            break;
        case CmdMsg::Type::STOP_TASK:
            resMsg = stopTask(msg);
            break;
        case CmdMsg::Type::DELETE_TASK:
            resMsg = deleteTask(msg);
            break;
        case CmdMsg::Type::GET_TASK:
            resMsg = getTask(msg);
            break;
        case CmdMsg::Type::GET_ALL_TASK:
            resMsg = getAllTask(msg);
            break;
        case CmdMsg::Type::ENABLE_REDIRECT:
            resMsg = enableIORedirect(msg);
            break;
        case CmdMsg::Type::DISABLE_REDIRECT:
            resMsg = disableIORedirect(msg);
            break;
        case CmdMsg::Type::PUT_TO_STDIN:
            resMsg = putToTaskInput(msg);
            break;
        case CmdMsg::Type::SHUT_DOWN:
            CmdMgr::stop();
            resMsg.status = CmdRes::Type::OK;
            break;
        default:
            fmt::print("handle cmdmsg: invalid msg type, ignored\n");
            resMsg.status = CmdRes::Type::INVALID_CMD_TYPE;
            break;
        }
        writeBackCmdRes(resMsg);
    } catch (const std::exception &e) {
        fmt::print("Handle CmdMsg failed, {}\n", e.what());
        writeBackCmdRes(CmdRes{.status = CmdRes::Type::FAILED});
    }
}

void onCmdSockReadCb(bufferevent *bev, void *arg) {
    char data[BUFSIZ];
    size_t n = 0;
    while ((n = bufferevent_read(bev, data, BUFSIZ)) > 0) {
        auto jsonList = CmdMgr::bufferHelper.tryFullFill(data, n);
        for (auto &&jsonObj : jsonList)
            handleCmdMsg(CmdMsg::parse(jsonObj));
    }
}

void onCmdSockEventCb(bufferevent *bev, short events, void *arg) {
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        fmt::print("CmdMgr: ERROR or EOF on cmdSock, shutting down CmdMgr\n");
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
            int32_t taskId = CmdMgr::runningTaskHelper.pid2RunningTask.at(pid)->mTaskId;
            CmdMgr::runningTaskHelper.removeByPid(pid);
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
