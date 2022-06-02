#if !defined(CMD_MGR)
#define CMD_MGR

#include "event2/event.h"
#include "nlohmann/json.hpp"
#include "task.hpp"
#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>

using nlohmann::json;
using std::array, std::shared_ptr, std::make_shared;
using std::string, std::map, std::runtime_error;

struct RunningTaskHelper;

struct CmdMgr {
    static int cmdSock;
    static event_base *base;
    static RunningTaskHelper runningTaskHelper;

    static void start(int cmdSock);
};

enum CmdType {
    START_TASK,
    CREATE_TASK,
    STOP_TASK,
    DELETE_TASK,
    GET_TASK,
    ENABLE_REDIRECT,
    DISABLE_REDIRECT,
    PUT_TO_STDIN,
};

struct CmdMsg {
    CmdType cmdType;
    int32_t taskId;
    string title; // for task create
    string scriptCode;
    TaskScriptType scriptType;
    int32_t interval;
    int32_t maxTimes;
    string stdinContent;

    string toJsonStr();
    // construct from json
    static CmdMsg parse(const json &);
};

struct CmdRes {
    bool ok;
    int32_t taskId;
    pid_t pid;
    Task task;
};

struct RunningTaskHelper {
private:
    struct RunningTask {
        int32_t taskId;
        int32_t pid;
        array<int, 3> fds;

        RunningTask(int32_t taskId, pid_t pid, int fdIn, int fdOut, int fdErr)
            : taskId(taskId), pid(pid), fds({fdIn, fdOut, fdErr}) {}
    };

    bool isPidIn(pid_t pid) { return pid2RunningTask.contains(pid); }
    bool isTaskIdIn(int32_t taskId) { return taskId2RunningTask.contains(taskId); }

public:
    map<pid_t, shared_ptr<RunningTask>> pid2RunningTask;
    map<int32_t, shared_ptr<RunningTask>> taskId2RunningTask;

    void add(int32_t taskId, pid_t pid, int fdIn, int fdOut, int fdErr) {
        if (isPidIn(pid) || isTaskIdIn(taskId))
            throw runtime_error("add to running task failed, pid or taskId already exist");
        shared_ptr<RunningTask> taskPtr = make_shared<RunningTask>(taskId, pid, fdIn, fdOut, fdErr);
        pid2RunningTask[pid] = taskPtr;
        taskId2RunningTask[taskId] = taskPtr;
    }

    bool isTaskRunning(int32_t taskId) { return isTaskIdIn(taskId); }

    array<int, 3> getFdsByPid(pid_t pid) {
        if (!isPidIn(pid))
            throw runtime_error("get fd by pid failed, pid not found");
        return pid2RunningTask[pid]->fds;
    }

    array<int, 3> getFdsByTaskId(int32_t taskId) {
        if (!isTaskIdIn(taskId))
            throw runtime_error("get fd by taskId failed, taskId not found");
        return taskId2RunningTask[taskId]->fds;
    }

    void removeByPid(pid_t pid) {
        if (!isPidIn(pid))
            throw runtime_error("get fd by pid failed, pid not found");
        int32_t taskId = pid2RunningTask[pid]->taskId;
        pid2RunningTask.erase(pid);
        taskId2RunningTask.erase(taskId);
    }
};

#endif // CMD_MGR
