#if !defined(TASK_)
#define TASK_

#include <string>

#include "nlohmann/json.hpp"

using nlohmann::json;
using std::string;

namespace task {
    enum ScriptType { PYTHON,
                      BASH,
                      UNINIT };

    enum Status { IDLE,
                  RUNNING,
                  UNINIT };

    const char *TASK_KEY_ID = "id";
    const char *TASK_KEY_TITLE = "title";
    const char *TASK_KEY_SCRIPTCODE = "scriptCode";
    const char *TASK_KEY_SCRIPTTYPE = "scriptType";
    const char *TASK_KEY_STATUS = "status";
    const char *TASK_KEY_PID = "pid";
    const char *TASK_KEY_INTERVALINSEC = "intervalInSec";
    const char *TASK_KEY_MAXTIMES = "maxTimes";
    const char *TASK_KEY_EXITCODE = "exitCode";
    const char *TASK_KEY_EXITTIMESTAMP = "exitTimeStamp";

    struct Task {
        int32_t id;
        string title;
        string scriptCode;
        ScriptType scriptType;
        Status status;
        int32_t pid;
        int32_t intervalInSec;
        int32_t maxTimes;
        int32_t timesExecuted;
        int32_t exitCode;
        int64_t exitTimeStamp;

        Task() {}

        Task(json data) {
            try {
                id = data[TASK_KEY_ID].get<int32_t>();
                title = data[TASK_KEY_TITLE].get<string>();
                scriptCode = data[TASK_KEY_SCRIPTCODE].get<string>();
                scriptType = data[TASK_KEY_SCRIPTTYPE].get<ScriptType>();
                status = data[TASK_KEY_STATUS].get<Status>();
                pid = data[TASK_KEY_PID].get<int32_t>();
                intervalInSec = data[TASK_KEY_INTERVALINSEC].get<int32_t>();
                maxTimes = data[TASK_KEY_MAXTIMES].get<int32_t>();
                exitCode = data[TASK_KEY_EXITCODE].get<int32_t>();
                exitTimeStamp = data[TASK_KEY_EXITTIMESTAMP].get<int64_t>();
            } catch (json::exception) {
                throw std::runtime_error("create task from json failed");
            }
        }

        json toJson() const {
            json res = {
                {TASK_KEY_ID, id},
                {TASK_KEY_TITLE, title},
                {TASK_KEY_SCRIPTCODE, scriptCode},
                {TASK_KEY_SCRIPTTYPE, scriptType},
                {TASK_KEY_STATUS, status},
                {TASK_KEY_PID, pid},
                {TASK_KEY_INTERVALINSEC, intervalInSec},
                {TASK_KEY_MAXTIMES, maxTimes},
                {TASK_KEY_EXITCODE, exitCode},
                {TASK_KEY_EXITTIMESTAMP, exitTimeStamp},
            };
            return res;
        }
    };

} // namespace task

#endif // TASK_
