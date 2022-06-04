#if !defined(CMD_MSG)
#define CMD_MSG

#include "nlohmann/json.hpp"

#include "task.hpp"

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

using nlohmann::json;
using std::string, std::vector, std::string_view;

struct CmdMsg {
    enum Type {
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
        _LAST,
    };
    Type cmdType;
    int32_t taskId;
    string title; // for task create
    string scriptCode;
    TaskScriptType scriptType;
    int32_t interval;
    int32_t maxTimes;
    string stdinContent;

    json toJson() const;
    string toJsonStr() const;
    // construct from json
    static CmdMsg parse(const json &);
};

constexpr string_view CMDTYPE_TO_STRING_VIEW[CmdMsg::Type::_LAST] = {
    "START_TASK",   "CREATE_TASK",     "STOP_TASK",        "DELETE_TASK",  "GET_TASK",
    "GET_ALL_TASK", "ENABLE_REDIRECT", "DISABLE_REDIRECT", "PUT_TO_STDIN", "SHUT_DOWN",
};

struct CmdRes {
    enum Type {
        OK,
        TASK_NOT_RUNNING,
        NO_SUCH_TASK,
        TASK_IS_RUNNING,
        FAILED,
        INVALID_CMD_TYPE,
        _LAST
    };
    Type status;
    int32_t taskId;
    pid_t pid;
    Task task;
    vector<Task> taskList;

    json toJson() const;
    string toJsonStr() const;
    static CmdRes parse(const json &);
};

constexpr string_view CMDRESTYPE_TO_STRING_VIEW[CmdRes::Type::_LAST] = {
    "OK", "TASK_NOT_RUNNING", "NO_SUCH_TASK", "TASK_IS_RUNNING", "FAILED", "INVALID_CMD_TYPE",
};

#endif // CMD_MSG
