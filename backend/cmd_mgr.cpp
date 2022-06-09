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
#include "util.hpp"

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
BufferHelper CmdMgr::bufferHelper;
RunningTaskHelper CmdMgr::runningTaskHelper;
ScheduledTaskHelper CmdMgr::schedTaskHelper;

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
    else if (task.scriptType == TaskScriptType::BASH)
        scriptRes = ScriptHelper::startBashScriptWithIORedir(filePath, fdsToClose);
    else {
        return CmdRes{.status = CmdRes::Type::FAILED};
    }

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
    if (CmdMgr::runningTaskHelper.isTaskRunning(msg.taskId))
        stopTask(msg);
    if (TaskDBHelper::hasTask(msg.taskId)) {
        TaskDBHelper::deleteTask(msg.taskId);
        event *ev = CmdMgr::schedTaskHelper.remove(msg.taskId);
        if (ev != nullptr)
            event_free(ev);
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
    terminateIfNot(bufferevent_write(CmdMgr::bevCmdSock, data.data(), data.size()) != -1);
}

void handleCmdMsg(const CmdMsg &msg, bool doWriteBack = true) {
    CmdRes resMsg;
    try {
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
            printlnTime("handle cmdmsg: invalid msg type, ignored");
            resMsg.status = CmdRes::Type::INVALID_CMD_TYPE;
            break;
        }
    } catch (const std::exception &e) {
        printlnTime("Handle CmdMsg failed, {}", e.what());
        resMsg = CmdRes{.status = CmdRes::Type::FAILED};
    }
    if (doWriteBack)
        writeBackCmdRes(resMsg);
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
        printlnTime("CmdMgr: ERROR or EOF on cmdSock, shutting down CmdMgr");
        bufferevent_free(bev);
        event_base_loopexit(CmdMgr::base, nullptr);
    }
}

void onTimerEvent(evutil_socket_t, short events, void *arg) {
    CmdMsg *msg = (CmdMsg *)arg;
    if (events & EV_TIMEOUT)
        handleCmdMsg(*msg, false);
    event *ev = CmdMgr::schedTaskHelper.remove(msg->taskId);
    if (ev != nullptr)
        event_free(ev);
    delete msg;
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
            // make sure task is not deleted
            if (TaskDBHelper::hasTask(taskId)) {
                Task newTask = TaskDBHelper::getTask(taskId);
                newTask.exitCode = WEXITSTATUS(status);
                newTask.exitTimeStamp = timestamp;
                newTask.status = TaskStatus::IDLE;
                newTask.pid = -1;
                TaskDBHelper::updateTask(taskId, newTask);
                if (newTask.exitCode == 0 && newTask.intervalInSec > 0 &&
                    newTask.timesExecuted < newTask.maxTimes) {
                    event *ev = evtimer_new(
                        CmdMgr::base, onTimerEvent,
                        (new CmdMsg{.cmdType = CmdMsg::Type::START_TASK, taskId = taskId}));
                    timeval tv{.tv_sec = newTask.intervalInSec, .tv_usec = 0};
                    evtimer_add(ev, &tv);
                    CmdMgr::schedTaskHelper.set(taskId, ev);
                }
            }
        }
    }
}

void CmdMgr::start(int sock) {
    evutil_make_socket_closeonexec(sock);
    CmdMgr::cmdSock = sock;
    CmdMgr::base = event_base_new();
    terminateIfNot(base != nullptr);

    // read on cmdSock
    CmdMgr::bevCmdSock =
        bufferevent_socket_new(CmdMgr::base, CmdMgr::cmdSock, BEV_OPT_CLOSE_ON_FREE);
    terminateIfNot(CmdMgr::bevCmdSock != nullptr);
    bufferevent_setcb(CmdMgr::bevCmdSock, onCmdSockReadCb, nullptr, onCmdSockEventCb, CmdMgr::base);
    terminateIfNot(bufferevent_enable(CmdMgr::bevCmdSock, EV_READ | EV_WRITE) != -1);

    // SIGCHILD
    CmdMgr::evChild = evsignal_new(CmdMgr::base, SIGCHLD, onSIGCHILDCb, CmdMgr::base);
    terminateIfNot(CmdMgr::evChild != nullptr);
    terminateIfNot(evsignal_add(CmdMgr::evChild, nullptr) != -1);

    // write 1 byte to let the parent know we are ready to go
    {
        while (true) {
            ssize_t res = write(CmdMgr::cmdSock, "1", 1);
            if (res == 1)
                break;
            if (res == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
                continue;
            else
                throw runtime_error("CmdMgr, write notification to CmdSock failed! " +
                                    string(strerror(errno)));
        }
    }

    terminateIfNot(event_base_loop(CmdMgr::base, EVLOOP_NO_EXIT_ON_EMPTY) != -1);
}

void CmdMgr::stop() {
    event_free(CmdMgr::evChild);
    bufferevent_free(CmdMgr::bevCmdSock);
    event_base_loopexit(CmdMgr::base, nullptr);
    event_base_free(CmdMgr::base);
}
