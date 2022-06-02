#if !defined(IO_MGR)
#define IO_MGR

#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "nlohmann/json.hpp"

#include <array>
#include <map>
#include <stdexcept>
#include <string>

using nlohmann::json;
using std::array, std::string;
using std::map, std::runtime_error;

struct TaskIOFdHelper;

struct IOMgr {
    static int ioSock;
    static event_base *base;
    static TaskIOFdHelper taskIOFdHelper;
    static bufferevent *bevIOSock;

    static void start(int ioSock);
    static void addFds(int32_t taskId, array<int, 3> fds);
    static void removeFds(array<int, 3> fds);
    static void enableRedirect(array<int, 2> fds);
    static void disableRedirect(array<int, 2> fds);
    static void putToStdin(int fd, string buf);
};

enum IOMsgType {
    ADD_FD,
    REMOVE_FD,
    ENABLE_FD_REDIRECT,
    DISABLE_FD_REDIRECT,
    PUT_DATA_TO_FD,
};

struct IODataMsg {
    int32_t taskId;
    int32_t outOrErr;
    string content;

    string toJson() const;
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

struct TaskIOFdHelper {
    struct IOData {
        int32_t taskId;
        int fd;
        bool redirectEnabled;
        bufferevent *bev;

        IOData(int32_t taskId, int fd, bufferevent *bev)
            : taskId(taskId), fd(fd), redirectEnabled(false), bev(bev) {}
    };

    map<int, IOData> fd2IOData;

    // check if fd is inside, throw if not
    void throwIfNotIn(int fd) {
        if (!fd2IOData.contains(fd))
            throw runtime_error("taskIOFdHelper: fd not found");
    }

    void add(int32_t taskId, int fd, bufferevent *bev) {
        if (fd2IOData.contains(fd))
            throw runtime_error("taskIOFdHelper: fd not found");
        fd2IOData[fd] = IOData(taskId, fd, bev);
    }

    bufferevent *removeFd(int fd) {
        bufferevent *bev = fd2IOData[fd].bev;
        fd2IOData.erase(fd);
        return bev;
    }

    void setRedirect(int fd, bool on) {
        throwIfNotIn(fd);
        fd2IOData[fd].redirectEnabled = on;
    }
    void enableRedirect(int fd) { setRedirect(fd, true); }
    void disableRedirect(int fd) { setRedirect(fd, false); }

    bool isRedirectEnabled(int fd) {
        throwIfNotIn(fd);
        return fd2IOData[fd].redirectEnabled;
    }

    bufferevent *getBufferEv(int fd) {
        throwIfNotIn(fd);
        return fd2IOData[fd].bev;
    }

    int32_t getTaskId(int fd) {
        throwIfNotIn(fd);
        return fd2IOData[fd].taskId;
    }
};

#endif // IO_MGR
