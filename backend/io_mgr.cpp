#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"

#include "buffer_helper.hpp"
#include "io_mgr.hpp"

#include <cassert>
#include <cstring>
#include <errno.h>
#include <unistd.h>

int IOMgr::ioSock = -1;
event_base *IOMgr::base = nullptr;
TaskIOFdHelper IOMgr::taskIOFdHelper;
bufferevent *IOMgr::bevIOSock = nullptr;

void onIOSockEventCb(bufferevent *bev, short events, void *arg) {
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        printf("Error on IO Sock, shutting down IOMgr\n");
        bufferevent_free(bev);
        event_base_loopexit((event_base *)(arg), nullptr);
        IOMgr::base = nullptr;
    }
}

void IOMgr::start(int sock) {
    evutil_make_socket_closeonexec(sock);
    IOMgr::ioSock = sock;
    IOMgr::base = event_base_new();
    assert(base != nullptr);
    IOMgr::bevIOSock = bufferevent_socket_new(IOMgr::base, IOMgr::ioSock, BEV_OPT_CLOSE_ON_FREE);
    assert(IOMgr::bevIOSock != nullptr);
    bufferevent_setcb(IOMgr::bevIOSock, nullptr, nullptr, onIOSockEventCb, IOMgr::base);
    bufferevent_enable(IOMgr::bevIOSock, EV_WRITE);
    int res = event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
    assert(res != -1);
}

void fdReadDelegate(bufferevent *bev, int outOrError) {
    string tmpContent;
    tmpContent.reserve(BUFSIZ);
    char buf[BUFSIZ + 1];
    size_t n;
    while ((n = bufferevent_read(bev, buf, BUFSIZ)) != 0) {
        tmpContent.append(buf, n);
    }
    int fd = bufferevent_getfd(bev);
    assert(fd != -1);
    string data = BufferHelper::make(IODataMsg{
        .taskId = IOMgr::taskIOFdHelper.getTaskId(fd),
        .outOrErr = outOrError,
        .content = std::move(tmpContent),
    }
                                         .toJson());

    int32_t res = bufferevent_write(IOMgr::bevIOSock, data.data(), data.size());
    if (res == -1)
        printf("IOMgr: bufferevent_write failed! ioSock is probably dead\n");
}

void onOutFdReadCb(bufferevent *bev, void *arg) { fdReadDelegate(bev, 0); }

void onFdEventCb(bufferevent *bev, short events, void *arg) {
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        printf("IOMgr: EOF or ERROR on fd %d\n", bufferevent_getfd(bev));
        bufferevent_free(bev);
    }
}

void onErrFdReadCb(bufferevent *bev, void *arg) { fdReadDelegate(bev, 1); }

void onMsgEventCb(evutil_socket_t, short events, void *arg) {
    IOEventMsg *msg = (IOEventMsg *)(arg);
    bufferevent *bev = nullptr;
    switch (msg->msgType) {
    case ADD_FD:
        if (msg->fdType == 0) {
            evutil_make_socket_nonblocking(msg->fd);
        } else {
            bev = bufferevent_socket_new(IOMgr::base, msg->fd, BEV_OPT_CLOSE_ON_FREE);
            assert(bev != nullptr);
            bufferevent_setcb(bev, (msg->fdType == 1 ? onOutFdReadCb : onErrFdReadCb), nullptr,
                              onFdEventCb, IOMgr::base);
            int res = bufferevent_enable(bev, EV_READ);
            assert(res != -1);
        }
        IOMgr::taskIOFdHelper.add(msg->taskId, msg->fd, bev);
        break;
    case REMOVE_FD:
        bev = IOMgr::taskIOFdHelper.removeFd(msg->fd);
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
            int32_t n = (int32_t)write(msg->fd, data + nWritten, nToWrite);
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
    }
    event_free(msg->ev);
    delete msg;
}

inline void addMsgEvent(IOEventMsg *msg) {
    if (IOMgr::base == nullptr)
        throw runtime_error("addMsgEvent error, event base has been shut down");
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

void IOMgr::stop() {
    bufferevent_free(IOMgr::bevIOSock);
    event_base_loopexit(IOMgr::base, nullptr);
    event_base_free(IOMgr::base);
}