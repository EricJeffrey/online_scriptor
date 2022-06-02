#include <array>
#include <cassert>
#include <fstream>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>

#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"

#include "unistd.h"

#include "script_helper.hpp"
#include "task.hpp"
#include "task_mgr.hpp"

using std::string, std::unordered_map, std::ofstream;

struct TaskInfo {
    int32_t taskId;
    pid_t taskPid;
    int fdStdin, fdStdout, fdStderr;
};
enum MsgCode {
    START,
    STOP,
    REDIRECT_IO,
    STOP_REDIRECT_IO,
    CHILD_EXITED,
};

struct ScriptMgrMsg {
    MsgCode msgCode;
    Task task;
    int pid;
    int exitCode;
    event *ev;

    ~ScriptMgrMsg() {
        event_del(ev);
    }
};

// TODO evthread_use_pthreads 会自动对 event_base 加锁
extern TaskMgr taskMgr;

unordered_map<pid_t, int32_t> taskIdOfPid;
unordered_map<int32_t, TaskInfo *> taskInfoOftaskId;
unordered_map<int, bool> ioRedirectOfFd;

constexpr char *SCRIPT_ROOT_PATH = "/data/online_scriptor/";
constexpr int32_t DEFAULT_BUF_SIZE = 1024;

// ----- libevent callback -----

void fdReadCallback(bufferevent *bev, void *arg) {
    int fd = bufferevent_getfd(bev);
    // bev一定会有绑定的 fd
    assert(fd != -1);
    // 绑定的 fd 一定在 ioRedirectOfFd 中
    assert(ioRedirectOfFd.find(fd) != ioRedirectOfFd.end());
    bool doRedirect = ioRedirectOfFd[fd];

    char buf[DEFAULT_BUF_SIZE + 1];
    buf[DEFAULT_BUF_SIZE] = 0;
    int n;
    evbuffer *input = bufferevent_get_input(bev);
    while ((n = bufferevent_read(bev, buf, DEFAULT_BUF_SIZE)) > 0) {
        if (doRedirect) {
            // TODO send to unixsock
            printf("script_mgr: will send to UnixSock here: %s\n", buf);
        } else {
            printf("script_mgr: will not send %s\n", buf);
        }
    }
}

void fdEventCallback(bufferevent *bev, short events, void *arg) {}

void eventCallback(evutil_socket_t fd, short events, void *arg) {
    ScriptMgrMsg *msg = static_cast<ScriptMgrMsg *>(arg);
    if (fd == -1 && (events & EV_TIMEOUT)) {
        switch (msg->msgCode) {
        case MsgCode::START:
            startTask(msg->task);
            break;
        case MsgCode::STOP:
            stopTask(msg->task);
            break;
        case MsgCode::REDIRECT_IO:
            startRedirectIO(msg->task);
            break;
        case MsgCode::STOP_REDIRECT_IO:
            stopRedirectIO(msg->task);
            break;
        case MsgCode::CHILD_EXITED:
            recycleChildInfo(msg->task.id, msg->pid, msg->exitCode);
            break;
        default:
            break;
        }
    }
    delete msg;
}

// ----- event handler -----

void addFdToBase(int fdStdin, int fdStdout, int fdStderr, TaskInfo *taskInfo) {
    bufferevent *bevStdou = bufferevent_socket_new(taskMgr.scriptMgrBase, fdStdout, BEV_OPT_CLOSE_ON_FREE);
    bufferevent *bevStderr = bufferevent_socket_new(taskMgr.scriptMgrBase, fdStderr, BEV_OPT_CLOSE_ON_FREE);
    bufferevent *bevStdin = bufferevent_socket_new(taskMgr.scriptMgrBase, fdStdin, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bevStdou, fdReadCallback, nullptr, fdEventCallback, taskInfo);
    bufferevent_enable(bevStdou, EV_READ);
    bufferevent_setcb(bevStderr, fdReadCallback, nullptr, fdEventCallback, taskInfo);
    bufferevent_enable(bevStderr, EV_READ);
}

void startTask(const Task &task) {
    const string filePath = SCRIPT_ROOT_PATH + std::to_string(task.id);
    ofstream ofs(filePath);
    ofs.write(task.scriptCode.c_str(), task.scriptCode.size());
    ofs.close();
    vector<int> fdsToClose;
    for (auto &&p : taskInfoOftaskId) {
        if (p.first != task.id) {
            fdsToClose.insert(fdsToClose.end(), {p.second->fdStdin, p.second->fdStdout, p.second->fdStderr});
        }
    }
    StartScriptRes res;
    if (task.scriptType == TaskScriptType::PYTHON)
        res = ScriptHelper::startPyScriptWithIORedir(filePath, fdsToClose);
    if (task.scriptType == TaskScriptType::BASH)
        res = ScriptHelper::startBashScriptWithIORedir(filePath, fdsToClose);

    // update
    taskIdOfPid[res.childPid] = task.id;
    ioRedirectOfFd[res.fdStdout] = false, ioRedirectOfFd[res.fdStderr] = false;
    // TODO delete on recycleChildInfo
    TaskInfo *taskInfo = new TaskInfo{
        .taskId = task.id,
        .taskPid = res.childPid,
        .fdStdin = res.fdStdin,
        .fdStdout = res.fdStdout,
        .fdStderr = res.fdStderr,
    };
    taskInfoOftaskId[task.id] = taskInfo;
    // add those fds to libevent
    addFdToBase(res.fdStdin, res.fdStdout, res.fdStderr, taskInfo);
    // TODO notify task started
    // notifyTaskStarted(task.id, res.childPid);
}

void stopTask(const Task &task) {}

void startRedirectIO(const Task &task) {}

void stopRedirectIO(const Task &task) {}

void writeToTaskSTDIN(const Task &task, const string &content) {}

void recycleChildInfo(int32_t taskId, int pid, int exitCode) {}

// ----- interface -----

void notifyStartScript(const Task &task) {
    if (task.scriptType != TaskScriptType::PYTHON && task.scriptType != TaskScriptType::BASH)
        return;
    ScriptMgrMsg *msg = new ScriptMgrMsg(); // 生命周期 evtimer_new -> eventCallback
    msg->msgCode = MsgCode::START;
    msg->task = task;
    event *ev = evtimer_new(taskMgr.scriptMgrBase, eventCallback, msg);
    assert(ev != nullptr);
    msg->ev = ev;
    timeval tv{.tv_sec = 0, .tv_usec = 0};
    event_add(ev, &tv);
}

void notifyStopScript(const Task &task) {}

void notifyRedirectIO(const Task &task) {}

void notifyPutToSTDIN(const Task &task, const string &content) {}

void notifyChildExited(pid_t childPid, int exitCode) {}

void startScriptMgr() {
    evutil_make_socket_closeonexec(taskMgr.unixSock);
    event_base_loop(taskMgr.scriptMgrBase, EVLOOP_NO_EXIT_ON_EMPTY);
}
