#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/thread.h"
#include "event2/util.h"

#include "nlohmann/json.hpp"

#include "leveldb/db.h"

#include "task.hpp"

using nlohmann::json;

using std::vector, std::optional, std::string;

typedef unsigned char byte;

// unsigned char string
using ustring = std::basic_string<byte>;

const size_t DEFAULT_BUF_SIZE = 1024;

// 封装从UnixSock读取到的数据
struct UnixSockDataHelper {
    ustring length; // char是有符号的，左移/右移可能产生负值
    string content;

    UnixSockDataHelper() {
        length.reserve(4);
        content.reserve(DEFAULT_BUF_SIZE);
    }

    // 将数据写入到content中，获取完整数据后解析为Json返回，同时清空content和length
    optional<json> tryFullFill(char *buf, size_t n) {
        size_t i = 0;
        while (i < n && length.size() < 4) {
            length.push_back(buf[i]);
            ++i;
        }
        if (length.size() >= 4) {
            size_t expectedSize =
                length[0] + (length[1] << 8) + (length[2] << 16) + (length[3] << 24);
            while (content.size() < expectedSize && i < n) {
                content.push_back(buf[i]);
            }
            if (content.size() == expectedSize) {
                json res = json::parse(content);
                length.clear();
                content.clear();
                return res;
            }
        }
        return std::nullopt;
    }
};

struct EventArg {
    event_base *base;
    UnixSockDataHelper *helper;
};

const int32_t OP_CREATE = 0;
const int32_t OP_START = 1;
const int32_t OP_STOP = 2;
const int32_t OP_DELETE = 3;
const int32_t OP_PUT_INPUT = 4;
const int32_t OP_REDIRECT_OUTPUT = 5;
const int32_t OP_STOP_REDIRECT_OUTPUT = 6;

// 数据格式 Operation = {taskId, opCode, content}

const char *USOCK_KEY_OP_CODE = "opCode";
const char *USOCK_KEY_TASK_ID = "taskId";
const char *USOCK_KYE_CONTENT = "content";

void createTask(const string &content) {
    
}

// 根据操作类型执行任务
void dispatchOp(const json &operation) {
    int32_t taskId = operation[USOCK_KEY_TASK_ID].get<int32_t>();
    int32_t opCode = operation[USOCK_KEY_OP_CODE].get<int32_t>();
    string content = operation[USOCK_KYE_CONTENT].get<string>();
    switch (opCode) {
    case OP_CREATE:
        // write to leveldb
        break;
    case OP_START:
        break;
    case OP_STOP:
        break;
    case OP_DELETE:
        break;
    case OP_PUT_INPUT:
        break;
    case OP_REDIRECT_OUTPUT:
        break;
    case OP_STOP_REDIRECT_OUTPUT:
        break;
    default:
        break;
    }
}

void unsock_readcb(bufferevent *bev, void *arg) {
    const int bufSize = 1024;
    char buf[bufSize];
    size_t n;
    while ((n = bufferevent_read(bev, buf, bufSize)) > 0) {
        auto res = ((EventArg *)arg)->helper->tryFullFill(buf, n);
        if (res.has_value()) {
            // dispatch
            dispatchOp(res.value());
        }
    }
}

void unsock_eventcb(bufferevent *bev, short events, void *arg) {
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        // error here
        event_base_loopexit(((EventArg *)arg)->base, nullptr);
    }
}

void startTaskMgr(int unixSock) {
    evthread_use_pthreads();

    event_base *base = event_base_new();
    UnixSockDataHelper pendingReq;
    EventArg arg{.base = base, .helper = &pendingReq};

    evutil_make_socket_nonblocking(unixSock);
    evutil_make_socket_closeonexec(unixSock);
    bufferevent *unixBufEv = bufferevent_socket_new(base, unixSock, 0);
    bufferevent_setcb(unixBufEv, unsock_readcb, nullptr, unsock_eventcb, &arg);
    bufferevent_enable(unixBufEv, EV_READ);

    event_base_dispatch(base);

    bufferevent_free(unixBufEv);
    event_base_free(base);
}

int main(int argc, char const *argv[]) {
    //
    // startTaskMgr(0);
    int x = 0x3323e8ff;
    char tt = 0xe8;
    std::cout << tt << std::endl;
    int y = tt + (tt << 8);
    std::cout << y << std::endl;
    std::cout << 232 + (232 << 8) << std::endl;

    std::cout << "Hello World" << std::endl;
    return 0;
}
