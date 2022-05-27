#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"
#include <cstdio>

timeval lasttime;
int ispersistent;

void timeout_cb(evutil_socket_t fd, short events, void *arg) {
    event *timeout = (event *)arg;

    timeval newtime;
    evutil_gettimeofday(&newtime, nullptr);
    timeval diff;
    evutil_timersub(&newtime, &lasttime, &diff);
    double elapsed = diff.tv_sec + diff.tv_usec / 1.0e6;

    printf("timeout_cb called at %ld: %.3f seconds elapsed.\n", newtime.tv_sec, elapsed);
    lasttime = newtime;

    if (!ispersistent) {
        timeval tv;
        evutil_timerclear(&tv);
        tv.tv_sec = 2;
        event_add(timeout, &tv);
    }
}

int main(int argc, char const *argv[]) {

    event_base *base = event_base_new();

    ispersistent = 1;
    short flags = EV_PERSIST;

    event timeout;
    event_assign(&timeout, base, -1, flags, timeout_cb, (void *)&timeout);

    timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = 3;
    event_add(&timeout, &tv);

    evutil_gettimeofday(&lasttime, nullptr);
    fprintf(stderr, "starting dispatch\n");
    event_base_dispatch(base);

    return 0;
}
