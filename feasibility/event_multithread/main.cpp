#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/thread.h"
#include "event2/util.h"

#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <thread>

using namespace std;

struct Arg {
    string s;
    int32_t v;
};
void cb(evutil_socket_t, short events, void *arg) {
    Arg *t = (Arg *)arg;
    if (events & EV_TIMEOUT)
        printf("from parent, s: %s, v: %d\n", t->s.c_str(), t->v);
}

void childJob(event_base *base) {
    printf("child, loop started\n");
    event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
    printf("child, loop stopped\n");
    event_base_free(base);
}

/*

所有的指针都要自己管理生命周期！！！

*/

int main(int argc, char const *argv[]) {
    evthread_use_pthreads();
    event_base *base = event_base_new();

    thread(childJob, base).detach();

    for (int i = 0; i < 10; i++) {
        sleep(1);
        printf("parent, sending %d\n", i);
        Arg arg{.s = "hello world", .v = i};
        // ！！！不应该用局部变量的地址
        event *ev = evtimer_new(base, cb, &arg);
        assert(ev != nullptr);
        timeval tv;
        tv.tv_usec = 100;
        // ！！！不应该用局部变量的地址
        event_add(ev, &tv);
    }

    return 0;
}
