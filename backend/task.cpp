
#include "task.hpp"

constexpr char TASK_KEY_ID[] = "id";
constexpr char TASK_KEY_TITLE[] = "title";
constexpr char TASK_KEY_SCRIPTCODE[] = "scriptCode";
constexpr char TASK_KEY_SCRIPTTYPE[] = "scriptType";
constexpr char TASK_KEY_STATUS[] = "status";
constexpr char TASK_KEY_PID[] = "pid";
constexpr char TASK_KEY_INTERVALINSEC[] = "intervalInSec";
constexpr char TASK_KEY_MAXTIMES[] = "maxTimes";
constexpr char TASK_KEY_EXITCODE[] = "exitCode";
constexpr char TASK_KEY_EXITTIMESTAMP[] = "exitTimeStamp";

Task::Task(const json& data) {
    try {
        id = data[TASK_KEY_ID].get<int32_t>();
        title = data[TASK_KEY_TITLE].get<string>();
        scriptCode = data[TASK_KEY_SCRIPTCODE].get<string>();
        scriptType = data[TASK_KEY_SCRIPTTYPE].get<TaskScriptType>();
        status = data[TASK_KEY_STATUS].get<TaskStatus>();
        pid = data[TASK_KEY_PID].get<int32_t>();
        intervalInSec = data[TASK_KEY_INTERVALINSEC].get<int32_t>();
        maxTimes = data[TASK_KEY_MAXTIMES].get<int32_t>();
        exitCode = data[TASK_KEY_EXITCODE].get<int32_t>();
        exitTimeStamp = data[TASK_KEY_EXITTIMESTAMP].get<int64_t>();
    } catch (const json::exception &e) {
        throw std::runtime_error("create task from json failed, " + string(e.what()));
    }
}
json Task::toJson() const {
    return json::object({
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
    });
}
