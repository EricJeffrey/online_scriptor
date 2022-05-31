#if !defined(SCRIPT_MGR)
#define SCRIPT_MGR

#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"

#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "task.hpp"

using task::Task;
using std::string;

struct TaskInfo {
    int32_t taskId;
    pid_t taskPid;
    int fdStdin, fdStdout, fdStderr;
    bufferevent *stdinBev, *stdoutBev, *stderrBev;
};
enum MsgCode {
    START,
    STOP,
    REDIRECT_IO,
    STOP_REDIRECT_IO
};

struct ScriptMgrMsg {
    MsgCode msgCode;
    Task task;
    int pid;
    event *ev;

    ~ScriptMgrMsg() {
        event_del(ev);
    }
};

void notifyStartScript(const Task &task);

void notifyStopScript(const Task &task);

void notifyRedirectIO(const Task &task);

void notifyPutToSTDIN(const Task &task, const string &content);

void notifyChildExited(pid_t childPid, int exitCode);

void startScriptMgr();

#endif // SCRIPT_MGR
