#if !defined(CMD_MSG)
#define CMD_MSG

#include "nlohmann/json.hpp"

#include "task.hpp"

#include <cctype>
#include <string>

using nlohmann::json;
using std::string;

enum CmdType {
    START_TASK,
    CREATE_TASK,
    STOP_TASK,
    DELETE_TASK,
    GET_TASK,
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

    string toJsonStr();
    // construct from json
    static CmdMsg parse(const json &);
};

#endif // CMD_MSG
