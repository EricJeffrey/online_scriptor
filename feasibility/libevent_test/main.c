#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void stdin_readcb(struct bufferevent *bev, void *ptr) {
    char buf[1024];
    int n;
    struct evbuffer *input = bufferevent_get_input(bev);
    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        fwrite("stdin got:", 1, 10, stdout);
        fwrite(buf, 1, n, stdout);
        fwrite("\n", 1, 1, stdout);
    }
}
void pipe_readcb(struct bufferevent *bev, void *ptr) {
    char buf[1024];
    int n;
    struct evbuffer *input = bufferevent_get_input(bev);
    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        fwrite("pipe got:", 1, 9, stdout);
        fwrite(buf, 1, n, stdout);
        fwrite("\n", 1, 1, stdout);
    }
}
void eventcb(struct bufferevent *bev, short events, void *ptr) {
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        printf("event eof or error\n");
        bufferevent_free(bev);
        event_base_loopexit((struct event_base *)ptr, NULL);
    }
}

int main(int argc, char const *argv[]) {
    int pipes[2];
    if (pipe(pipes) == -1) {
        printf("pipe failed\n");
        return 1;
    }
    pid_t childpid = fork();
    if (childpid == -1) {
        printf("fork failed\n");
        return 1;
    } else if (childpid == 0) { // child
        close(pipes[0]);
        char buf[1024];
        buf[0] = '1';
        if (write(pipes[1], buf, 1) == -1) {
            printf("child write failed\n");
            return 1;
        }
        strncpy(buf, "hello world: ", 13);
        buf[14] = 0;
        for (int i = 0; i < 26; i++) {
            sleep(1);
            buf[13] = (char)(i + 'a');
            if (write(pipes[1], buf, 14) == -1) {
                printf("child write failed 1\n");
                return 1;
            }
        }
        close(pipes[1]);
        return 0;
    } else { // parent
        close(pipes[1]);
        struct event_base *base = event_base_new();

        struct bufferevent *stdin_bev = bufferevent_socket_new(base, 0, 0);
        bufferevent_setcb(stdin_bev, stdin_readcb, NULL, eventcb, base);
        bufferevent_enable(stdin_bev, EV_READ);

        struct bufferevent *pipe_bev = bufferevent_socket_new(base, pipes[0], 0);
        bufferevent_setcb(pipe_bev, pipe_readcb, NULL, eventcb, base);
        bufferevent_enable(pipe_bev, EV_READ);

        event_base_dispatch(base);
        printf("after dispatch\n");
        wait(NULL);
    }
    return 0;
}
