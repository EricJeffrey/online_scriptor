#include "io_mgr.hpp"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"
#include <cassert>
#include <cstring>
#include <errno.h>
#include <unistd.h>

int IOMgr::ioSock = -1;
event_base *IOMgr::base = nullptr;
TaskIOFdHelper IOMgr::taskIOFdHelper;

void IOMgr::start(int unixSock) {
    base = event_base_new();
    assert(base != nullptr);
    assert(event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY) != -1);
}

void onOutFdReadCb(bufferevent *bev, void *arg) {}

void onOutFdEventCb(bufferevent *bev, short events, void *arg) {}

void onErrFdReadCb(bufferevent *bev, void *arg) {}

void onErrFdEventCb(bufferevent *bev, short events, void *arg) {}

void onMsgEventCb(evutil_socket_t, short events, void *arg) {
    IOEventMsg *msg = (IOEventMsg *)(arg);
    switch (msg->msgType) {
    case ADD_FD:
        bufferevent *bev = nullptr;
        if (msg->fdType == 0) {
            evutil_make_socket_nonblocking(msg->fd);
        } else {
            bev = bufferevent_socket_new(IOMgr::base, msg->fd, BEV_OPT_CLOSE_ON_FREE);
            assert(bev != nullptr);
            bufferevent_setcb(bev, (msg->fdType == 1 ? onOutFdReadCb : onErrFdReadCb), nullptr,
                              (msg->fdType == 1 ? onOutFdEventCb : onErrFdEventCb), IOMgr::base);
            int res = bufferevent_enable(bev, EV_READ);
            assert(res != -1);
        }
        IOMgr::taskIOFdHelper.add(msg->taskId, msg->fd, bev);
        break;
    case REMOVE_FD:
        bufferevent *bev = IOMgr::taskIOFdHelper.removeFd(msg->fd);
        if (bev != nullptr)
            bufferevent_free(bev);
        break;
    case ENABLE_FD_REDIRECT:
        IOMgr::taskIOFdHelper.enableRedirect(msg->fd);
        break;
    case DISABLE_FD_REDIRECT:
        IOMgr::taskIOFdHelper.disableRedirect(msg->fd);
        break;
    case PUT_DATA_TO_FD:
        IOMgr::taskIOFdHelper.throwIfNotIn(msg->fd);
        size_t nWritten = 0;
        size_t dataSize = msg->content.size();
        char *data = msg->content.data();
        while (nWritten < dataSize) {
            size_t nToWrite = dataSize - nWritten;
            int32_t n = write(msg->fd, data + nWritten, nToWrite);
            if (n == -1) {
                int tmpErrno = errno;
                if (tmpErrno == EWOULDBLOCK) {
                    printf("IOMgr put data to stdin, task is not reading, dropped\n");
                    break;
                } else if (tmpErrno != EINTR) {
                    printf("IOMgr put data to stdin, write to task stdin failed %s\n",
                           strerror(tmpErrno));
                    break;
                }
            }
            nWritten += n;
        }
        break;
    default:
        break;
    }
    event_free(msg->ev);
    delete msg;
}

inline void addMsgEvent(IOEventMsg *msg) {
    event *ev = event_new(IOMgr::base, -1, EV_TIMEOUT, onMsgEventCb, msg);
    assert(ev != nullptr);
    msg->ev = ev;
    timeval tv{.tv_sec = 0, .tv_usec = 0};
    event_add(ev, &tv);
}

void IOMgr::addFds(int32_t taskId, array<int, 3> fds) {
    int res = evutil_make_socket_nonblocking(fds[0]);
    assert(res != -1);
    for (int32_t i = 0; i < 3; i++) {
        IOEventMsg *msg = new IOEventMsg{
            .msgType = IOMsgType::ADD_FD,
            .taskId = taskId,
            .fd = fds[i],
            .fdType = i,
        };
        addMsgEvent(msg);
    }
}

void IOMgr::removeFds(array<int, 3> fds) {
    for (int32_t i = 0; i < 3; i++) {
        IOEventMsg *msg = new IOEventMsg{
            .msgType = IOMsgType::REMOVE_FD,
            .fd = fds[i],
        };
        addMsgEvent(msg);
    }
}

void IOMgr::enableRedirect(array<int, 2> fds) {
    for (int32_t i = 0; i < 2; i++) {
        IOEventMsg *msg = new IOEventMsg{
            .msgType = IOMsgType::ENABLE_FD_REDIRECT,
            .fd = fds[i],
            .fdType = i + 1,
        };
        addMsgEvent(msg);
    }
}

void IOMgr::disableRedirect(array<int, 2> fds) {
    for (int32_t i = 0; i < 2; i++) {
        IOEventMsg *msg = new IOEventMsg{
            .msgType = IOMsgType::DISABLE_FD_REDIRECT,
            .fd = fds[i],
            .fdType = i + 1,
        };
        addMsgEvent(msg);
    }
}

void IOMgr::putToStdin(int fd, string buf) {
    IOEventMsg *msg = new IOEventMsg{
        .msgType = IOMsgType::PUT_DATA_TO_FD,
        .fd = fd,
        .fdType = 0,
        .content = std::move(buf),
    };
    addMsgEvent(msg);
}