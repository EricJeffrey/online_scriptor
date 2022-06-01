
#include "event2/buffer.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"

#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>

void childJob(int t = 2) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 10; j++) {
            printf("Hello World,");
        }
        printf("\n");
        sleep(t);
    }
    printf("dying\n");
    fflush(stdout);
}

void sigcb(evutil_socket_t, short events, void *arg) {
    if (events & EV_SIGNAL) {
        printf("sigcb: %s\n", (char *)arg);
        int status = 0;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            printf("sigcb: child %d exited, status: %d\n", pid, WEXITSTATUS(status));
        }
    }
}

int main(int argc, char const *argv[]) {
    pid_t child1Pid = fork();
    assert(child1Pid != -1);
    if (child1Pid == 0) { // child
        childJob();
        exit(0);
    }
    printf("child %d started\n", child1Pid);
    pid_t child2Pid = fork();
    assert(child2Pid != -1);
    if (child2Pid == 0) { // child
        childJob(3);
        exit(1);
    }
    printf("child %d started\n", child2Pid);
    event_base *base = event_base_new();
    assert(base != nullptr);
    event *ev = event_new(base, SIGCHLD, EV_SIGNAL | EV_PERSIST, sigcb, (char *)"CHILD DONE");
    assert(ev != nullptr);
    assert(event_add(ev, nullptr) != -1);
    event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
    return 0;
}
