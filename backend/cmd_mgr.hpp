#if !defined(CMD_MGR)
#define CMD_MGR

#include "buffer_helper.hpp"
#include "task.hpp"

#include "event2/bufferevent.h"
#include "event2/event.h"

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using std::array, std::shared_ptr;
using std::string, std::map, std::runtime_error, std::vector;

struct RunningTaskHelper;

struct CmdMgr {
    static int cmdSock;
    static event_base *base;
    static event *evChild;
    static bufferevent *bevCmdSock;
    static RunningTaskHelper runningTaskHelper;
    static BufferHelper bufferHelper;

    static void start(int cmdSock);
    static void stop();
};

struct RunningTaskHelper {
private:
    struct RunningTask {
        int32_t mTaskId;
        int32_t mPid;
        array<int, 3> mFds;

        RunningTask(int32_t taskId, pid_t pid, int fdIn, int fdOut, int fdErr)
            : mTaskId(taskId), mPid(pid), mFds({fdIn, fdOut, fdErr}) {}
    };

    bool isPidIn(pid_t pid) { return pid2RunningTask.find(pid) != pid2RunningTask.end(); }
    bool isTaskIdIn(int32_t taskId) {
        return taskId2RunningTask.find(taskId) != taskId2RunningTask.end();
    }

public:
    map<pid_t, shared_ptr<RunningTask>> pid2RunningTask;
    map<int32_t, shared_ptr<RunningTask>> taskId2RunningTask;

    void add(int32_t taskId, pid_t pid, int fdIn, int fdOut, int fdErr) {
        if (isPidIn(pid) || isTaskIdIn(taskId))
            throw runtime_error("add to running task failed, pid or taskId already exist");
        shared_ptr<RunningTask> taskPtr =
            std::make_shared<RunningTask>(taskId, pid, fdIn, fdOut, fdErr);
        pid2RunningTask[pid] = taskPtr;
        taskId2RunningTask[taskId] = taskPtr;
    }

    bool isTaskRunning(int32_t taskId) { return isTaskIdIn(taskId); }

    array<int, 3> getFdsByPid(pid_t pid) {
        if (!isPidIn(pid))
            throw runtime_error("get fd by pid failed, pid not found");
        return pid2RunningTask.at(pid)->mFds;
    }

    array<int, 3> getFdsByTaskId(int32_t taskId) {
        if (!isTaskIdIn(taskId))
            throw runtime_error("get fd by taskId failed, taskId not found");
        return taskId2RunningTask[taskId]->mFds;
    }

    void removeByPid(pid_t pid) {
        if (!isPidIn(pid))
            throw runtime_error("get fd by pid failed, pid not found");
        int32_t taskId = pid2RunningTask.at(pid)->mTaskId;
        pid2RunningTask.erase(pid);
        taskId2RunningTask.erase(taskId);
    }

    vector<int> getFdsExcept(int32_t taskId) {
        vector<int> fds;
        for (auto &&p : taskId2RunningTask)
            if (p.first != taskId)
                fds.insert(fds.end(), p.second->mFds.begin(), p.second->mFds.end());
        return fds;
    }
};

#endif // CMD_MGR
