#if !defined(CMD_MGR)
#define CMD_MGR

#include "nlohmann/json.hpp"
#include "task.hpp"
#include <string>

using nlohmann::json;
using std::string;

struct CmdMgr {
    static int cmdSock;
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
    string content;
    // for create task
    string title;
    string scriptCode;
    TaskScriptType scriptType;
    int32_t interval;
    int32_t maxTimes;

    CmdMsg() {}
    CmdMsg(const json &) {}
};

#endif // CMD_MGR
