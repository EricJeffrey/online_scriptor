#if !defined(CMD_MSG)
#define CMD_MSG

#include "nlohmann/json.hpp"

#include "task.hpp"

#include <cctype>
#include <string>
#include <vector>

using nlohmann::json;
using std::string, std::vector;

enum CmdType {
    START_TASK,
    CREATE_TASK,
    STOP_TASK,
    DELETE_TASK,
    GET_TASK,
    GET_ALL_TASK,
    ENABLE_REDIRECT,
    DISABLE_REDIRECT,
    PUT_TO_STDIN,
    SHUT_DOWN,
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

    string toJsonStr() const;
    // construct from json
    static CmdMsg parse(const json &);
};

enum CmdResType {
    OK, TASK_NOT_RUNNING, NO_SUCH_TASK, TASK_IS_RUNNING, FAILED,  INVALID_CMD_TYPE
};

struct CmdRes {
    CmdResType status;
    int32_t taskId;
    pid_t pid;
    Task task;
    vector<Task> taskList;

    string toJson() const;
};

#endif // CMD_MSG
