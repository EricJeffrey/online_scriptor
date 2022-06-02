#if !defined(IO_MGR)
#define IO_MGR

#include "event2/buffer.h"
#include "event2/bufferevent.h"

#include <array>
#include <map>
#include <stdexcept>
#include <string>

using std::array, std::string;
using std::map, std::runtime_error;

struct TaskIOFdHelper;

struct IOMgr {
    static int ioSock;
    static event_base *base;
    static TaskIOFdHelper taskIOFdHelper;

    static void start(int ioSock);
    static void addFds(array<int, 3> fds);
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

struct IOMsg {
    IOMsgType msgType;
    int fd;
    string buf;
};

struct TaskIOFdHelper {
    struct IOData {
        int fd;
        bool redirectEnabled;
        bufferevent *bev;

        IOData(int fd, bufferevent *bev) : fd(fd), redirectEnabled(false), bev(bev) {}
    };

    map<int, IOData> fd2IOData;

    void throwIfNotIn(int fd) {
        if (!fd2IOData.contains(fd))
            throw runtime_error("taskIOFdHelper: fd not found");
    }

    void add(int fd, bufferevent *bev) {
        if (fd2IOData.contains(fd))
            throw runtime_error("taskIOFdHelper: fd not found");
        fd2IOData[fd] = IOData(fd, bev);
    }

    bufferevent *removeFd(int fd) {
        throwIfNotIn(fd);
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
};

#endif // IO_MGR
