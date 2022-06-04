#include "io_mgr.hpp"

constexpr char IODATAMSG_KEY_TASK_ID[] = "taskId";
constexpr char IODATAMSG_KEY_CONTENT[] = "content";
constexpr char IODATAMSG_KEY_OUT_OR_ERROR[] = "outOrError";

string IODataMsg::toJsonStr() const {
    return json::object({
        {IODATAMSG_KEY_TASK_ID, taskId},
        {IODATAMSG_KEY_CONTENT, content},
        {IODATAMSG_KEY_OUT_OR_ERROR, outOrErr},
    }).dump();
}

IODataMsg IODataMsg::parse(const json &jsonStr) {
    if (jsonStr.contains(IODATAMSG_KEY_TASK_ID) && jsonStr.contains(IODATAMSG_KEY_CONTENT)) {
        return IODataMsg{
            .taskId = jsonStr[IODATAMSG_KEY_TASK_ID].get<int32_t>(),
            .outOrErr = jsonStr[IODATAMSG_KEY_OUT_OR_ERROR].get<int32_t>(),
            .content = jsonStr[IODATAMSG_KEY_CONTENT].get<string>(),
        };
    }
    throw runtime_error("IODataMsg.parse invalid json string");
}