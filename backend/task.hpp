#if !defined(TASK)
#define TASK

#include "nlohmann/json.hpp"
#include <string>

using nlohmann::json;
using std::string;

enum TaskScriptType { PYTHON, BASH, UNINIT };

enum TaskStatus { IDLE, RUNNING, UNINIT };

struct Task {
    int32_t id;
    string title;
    string scriptCode;
    TaskScriptType scriptType;
    TaskStatus status;
    int32_t pid;
    int32_t intervalInSec;
    int32_t maxTimes;
    int32_t timesExecuted;
    int32_t exitCode;
    int64_t exitTimeStamp;

    Task() {}

    Task(json data);

    json toJson() const;
};

#endif // TASK
