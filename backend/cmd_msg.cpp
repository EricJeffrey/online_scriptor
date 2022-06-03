#include "cmd_msg.hpp"

using std::runtime_error;

constexpr char *CMDMSG_KEY_CMDTYPE = "cmdType";
constexpr char *CMDMSG_KEY_TASKID = "taskId";
constexpr char *CMDMSG_KEY_STDINCONTENT = "stdinContent";
constexpr char *CMDMSG_KEY_TITLE = "title";
constexpr char *CMDMSG_KEY_SCRIPTCODE = "scriptCode";
constexpr char *CMDMSG_KEY_SCRIPTTYPE = "scriptType";
constexpr char *CMDMSG_KEY_INTERVAL = "interval";
constexpr char *CMDMSG_KEY_MAXTIMES = "maxTimes";

string CmdMsg::toJsonStr() {
    return json({
        {CMDMSG_KEY_CMDTYPE, cmdType},
        {CMDMSG_KEY_TASKID, taskId},
        {CMDMSG_KEY_TITLE, title},
        {CMDMSG_KEY_SCRIPTCODE, scriptCode},
        {CMDMSG_KEY_SCRIPTTYPE, scriptType},
        {CMDMSG_KEY_INTERVAL, interval},
        {CMDMSG_KEY_MAXTIMES, maxTimes},
        {CMDMSG_KEY_STDINCONTENT, stdinContent},
    });
}

CmdMsg CmdMsg::parse(const json &jsonStr) {
    auto checkKeyWithThrow = [&jsonStr](const char *key) {
        if (!jsonStr.contains(key))
            throw runtime_error("construct CmdMsg from json failed, key " + string(key) +
                                " is necessary");
    };
    checkKeyWithThrow(CMDMSG_KEY_CMDTYPE);
    CmdType cmdType = jsonStr[CMDMSG_KEY_CMDTYPE].get<CmdType>();
    CmdMsg resMsg{.cmdType = cmdType};

    switch (cmdType) {
    case CREATE_TASK:
        checkKeyWithThrow(CMDMSG_KEY_TITLE);
        checkKeyWithThrow(CMDMSG_KEY_SCRIPTCODE);
        checkKeyWithThrow(CMDMSG_KEY_SCRIPTTYPE);
        checkKeyWithThrow(CMDMSG_KEY_INTERVAL);
        checkKeyWithThrow(CMDMSG_KEY_MAXTIMES);
        resMsg.title = jsonStr[CMDMSG_KEY_TITLE].get<string>();
        resMsg.scriptCode = jsonStr[CMDMSG_KEY_SCRIPTCODE].get<string>();
        resMsg.scriptType = jsonStr[CMDMSG_KEY_SCRIPTTYPE].get<TaskScriptType>();
        resMsg.interval = jsonStr[CMDMSG_KEY_INTERVAL].get<int32_t>();
        resMsg.maxTimes = jsonStr[CMDMSG_KEY_MAXTIMES].get<int32_t>();
        break;
    case START_TASK:
    case STOP_TASK:
    case DELETE_TASK:
    case GET_TASK:
    case ENABLE_REDIRECT:
    case DISABLE_REDIRECT:
        checkKeyWithThrow(CMDMSG_KEY_TASKID);
        resMsg.taskId = jsonStr[CMDMSG_KEY_TASKID].get<int32_t>();
        break;
    case PUT_TO_STDIN:
        checkKeyWithThrow(CMDMSG_KEY_TASKID);
        checkKeyWithThrow(CMDMSG_KEY_STDINCONTENT);
        resMsg.taskId = jsonStr[CMDMSG_KEY_TASKID].get<int32_t>();
        resMsg.stdinContent = jsonStr[CMDMSG_KEY_TASKID].get<string>();
        break;
    case SHUT_DOWN:
        break;
    default:
        throw runtime_error("construct CmdMsg failed, invalid cmdTyp");
        break;
    }
    return resMsg;
}
