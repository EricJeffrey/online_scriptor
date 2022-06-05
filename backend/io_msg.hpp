#if !defined(IO_MSG)
#define IO_MSG

#include "event2/event.h"
#include "nlohmann/json.hpp"

#include <string>

using nlohmann::json;
using std::string;

enum IOMsgType
{
    ADD_FD,
    REMOVE_FD,
    ENABLE_FD_REDIRECT,
    DISABLE_FD_REDIRECT,
    PUT_DATA_TO_FD,
};

// Redirected IO Data, outOrErr=0 for STDOUT, 1 for STDERR
struct IODataMsg {
    int32_t taskId;
    // 0 for out, 1 for error
    int32_t outOrErr;
    string content;

    string toJsonStr() const;
    static IODataMsg parse(const json &);
};

struct IOEventMsg {
    event *ev;
    IOMsgType msgType;
    int32_t taskId;
    int fd;
    // 0 stdin, 1 stdout, 2 stderr
    int32_t fdType;
    string content;
};

#endif // IO_MSG
