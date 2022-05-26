
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/util.h"
#include <event2/thread.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

using namespace std;

void child_readcb(bufferevent *bev, void *ptr) {
    char buf[1024];
    int n;
    evbuffer *input = bufferevent_get_input(bev);
    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        cout << "child, got parent msg: " << buf << ", n: " << n << endl;
    }
}
void child_eventcb(bufferevent *bev, short events, void *ptr) {
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        cerr << "child, event error, exiting" << endl;
        event_base_loopexit((event_base *)ptr, nullptr);
    }
}

void childJob(int socket) {
    // !!! make libevent know thread, so that we can call exit in another thread
    evthread_use_pthreads();
    event_base *base = event_base_new();

    thread(
        [](int sock, event_base *base) {
            char buf[128] = "hello from child, ";
            buf[19] = 0;
            for (int i = 0; i < 3; i++) {
                buf[18] = i + 'a';
                if (write(sock, buf, strlen(buf)) == -1) {
                    cerr << "child: write failed, " << strerror(errno) << endl;
                    return;
                }
                sleep(1);
            }
            cout << "child, thread done" << endl;
            event_base_loopexit(base, nullptr);
        },
        socket, base)
        .detach();

    bufferevent *bev = bufferevent_socket_new(base, socket, 0);
    bufferevent_setcb(bev, child_readcb, nullptr, child_eventcb, base);
    bufferevent_enable(bev, EV_READ);
    event_base_dispatch(base);
    cout << "child, after dispatch" << endl;
    close(socket);
    event_base_free(base);
}

struct Tmp {
    event_base *base;
    int sock;
};

void read_cb(bufferevent *bev, void *ptr) {
    char buf[1024];
    char obuf[1024];
    int n;
    evbuffer *input = bufferevent_get_input(bev);
    evbuffer *output = bufferevent_get_output(bev);
    while ((n = evbuffer_remove(input, buf, sizeof(buf) - 2)) > 0) {
        cout << "parent, got child msg: " << buf << ", n: " << n << endl;
        memcpy(obuf, buf, n);
        obuf[n] = '!';
        evbuffer_add(output, obuf, n + 1);
    }
}
void event_cb(bufferevent *bev, short events, void *ptr) {
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        cerr << "parent, event error, exiting" << endl;
        event_base_loopexit(((Tmp *)ptr)->base, nullptr);
    }
}

int main(int argc, char const *argv[]) {
    int sockfds[2] = {};
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sockfds) == -1) {
        cerr << "socketpair failed" << endl;
        return 1;
    }

    pid_t childpid = fork();
    if (childpid == -1) {
        cerr << "fork failed" << endl;
        return 1;
    } else if (childpid == 0) { // child
        close(sockfds[0]);
        childJob(sockfds[1]);
        exit(0);
    } else {
        close(sockfds[1]);

        event_base *base = event_base_new();
        bufferevent *bev = bufferevent_socket_new(base, sockfds[0], 0);
        Tmp tmp{.base = base, .sock = sockfds[0]};

        bufferevent_setcb(bev, read_cb, nullptr, event_cb, &tmp);
        bufferevent_enable(bev, EV_READ);
        event_base_dispatch(base);
        close(sockfds[0]);
        cout << "parent, after dispatch, wait and exiting" << endl;
        event_base_free(base);
        wait(nullptr);
    }
    return 0;
}
