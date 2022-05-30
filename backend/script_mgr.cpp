#include <array>
#include <cassert>
#include <fstream>
#include <ostream>
#include <string>
#include <tuple>

#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"

#include "unistd.h"

#include "task_mgr.hpp"

using std::string, std::array, std::ofstream;

extern TaskMgr taskMgr;

enum MsgCode {
    START,
    STOP,
    REDIRECT_IO,
    STOP_REDIRECT_IO
};

struct ScriptMgrMsg {
    MsgCode msgCode;
    int32_t taskId;
    int pid;
    string script;
    event *ev;

    ~ScriptMgrMsg() {
        event_del(ev);
    }
};

constexpr char *PATH = "/";

array<int, 3> startTask(int32_t taskId, const string &content) {
    ofstream ofs("/data/online_scriptor/" + std::to_string(taskId));
    ofs.write(content.c_str(), content.size());
    ofs.close();
    pid_t child = fork();
    if (child == -1) {
        throw std::runtime_error("fork failed!");
    } else if (child == 0) {
        // script interpretor
    } else {
    }
}

void event_cb(evutil_socket_t, short events, void *arg) {
    ScriptMgrMsg *msg = static_cast<ScriptMgrMsg *>(arg);
    if (events & EV_TIMEOUT) {
        if (msg->msgCode == MsgCode::START) {
            startTask(msg->taskId, msg->script);
        }
    }
    delete msg;
}

void notifyStartScript(int32_t taskId, const string &script) {
    ScriptMgrMsg *msg = new ScriptMgrMsg();
    msg->msgCode = MsgCode::START;
    msg->taskId = taskId;
    msg->script = script;
    event *ev = evtimer_new(taskMgr.scriptMgrBase, event_cb, msg);
    assert(ev != nullptr);
    msg->ev = ev;
    timeval tv{.tv_sec = 0, .tv_usec = 0};
    event_add(ev, &tv);
}

void startScriptMgr() {
    evutil_make_socket_closeonexec(taskMgr.unixSock);
    event_base_loop(taskMgr.scriptMgrBase, EVLOOP_NO_EXIT_ON_EMPTY);
}
