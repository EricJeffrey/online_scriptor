#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char outbuf[64];
int outi = 0;

char errbuf[64];
int erri = 0;

struct Tmp {
    struct event_base *base;
    int child_stdin_fd;
};

void stdout_readcb(struct bufferevent *bev, void *ptr) {
    char buf[1024];
    int n;
    struct evbuffer *input = bufferevent_get_input(bev);
    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        printf("parent read child stdout %d:", n);
        fwrite(buf, 1, n, stdout);
        printf("\n");
        for (int i = 0; i < n; i++) {
            outbuf[outi] = buf[i];
            outi++;
            if (outi >= sizeof(outbuf))
                outi = 0;
        }
    }
    char ch = rand() % 26 + 'a';
    if (write(((struct Tmp *)ptr)->child_stdin_fd, &ch, 1) == -1) {
        if (errno != EAGAIN)
            printf("parent write to child failed\n");
    } else {
        printf("parent write %c to child\n", ch);
    }
}
void stderr_readcb(struct bufferevent *bev, void *ptr) {
    char buf[1024];
    int n;
    struct evbuffer *input = bufferevent_get_input(bev);
    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        printf("parent read child stderr %d:", n);
        fwrite(buf, 1, n, stdout);
        printf("\n");
        for (int i = 0; i < n; i++) {
            errbuf[erri] = buf[i];
            erri++;
            if (erri >= sizeof(errbuf))
                erri = 0;
        }
    }
}
void stdin_writecb(struct bufferevent *bev, void *ptr) {
    struct evbuffer *output = bufferevent_get_output(bev);
    evbuffer_add(output, "1", 1);
    printf("parent write 1 to child stdin\n");
}
void eventcb(struct bufferevent *ev, short events, void *ptr) {
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        printf("event ERROR or EOF\n");
        event_base_loopexit(((struct Tmp *)ptr)->base, NULL);
    }
}

void childJob() {
    for (int i = 0; i < 26; i++) {
        char ch = i + 'a';
        if (write(1, "hello stdout world:", 19) == -1 ||
            write(1, &ch, 1) == -1 ||
            read(0, &ch, 1) == -1 ||
            write(2, "hello stderr world:", 19) == -1 ||
            write(2, &ch, 1) == -1)
            return;
        sleep(1);
    }
}

int main(int argc, char const *argv[]) {
    int stdinp[2], stdoutp[2], stderrp[2];
    if (pipe2(stdinp, 0) == -1 ||
        pipe2(stdoutp, 0) == -1 ||
        pipe2(stderrp, 0) == -1) {
        printf("pipe failed\n");
        return 1;
    }
    pid_t childpid = fork();
    if (childpid == -1) {
        printf("fork failed\n");
        return 1;
    } else if (childpid == 0) {
        printf("child started (before dup)\n");
        close(stdinp[1]), close(stdoutp[0]), close(stderrp[0]);
        // child
        if (dup2(stdinp[0], 0) == -1 ||
            dup2(stdoutp[1], 1) == -1 ||
            dup2(stderrp[1], 2) == -1) {
            printf("child dup2 failed\n");
            return 1;
        }
        childJob();
        close(stdinp[0]), close(stdoutp[1]), close(stderrp[1]);
        return 0;
    } else {
        // parent
        printf("parent started, child: %d\n", childpid);
        close(stdinp[0]), close(stdoutp[1]), close(stderrp[1]);

        struct event_base *base = event_base_new();
        struct Tmp tmp = {.base = base, .child_stdin_fd = stdinp[1]};

        // !!! Data written to the write end of the pipe is buffered by the kernel until it is read from the read end of the pipe
        struct bufferevent *ibev = bufferevent_socket_new(base, stdinp[1], 0);
        bufferevent_enable(ibev, EV_WRITE);
        bufferevent_setcb(ibev, NULL, stdin_writecb, eventcb, &tmp);

        struct bufferevent *obev = bufferevent_socket_new(base, stdoutp[0], 0);
        bufferevent_enable(obev, EV_READ);
        bufferevent_setcb(obev, stdout_readcb, NULL, eventcb, &tmp);

        struct bufferevent *ebev = bufferevent_socket_new(base, stderrp[0], 0);
        bufferevent_enable(ebev, EV_READ);
        bufferevent_setcb(ebev, stderr_readcb, NULL, eventcb, &tmp);

        event_base_dispatch(base);

        printf("after dispatch\n");
        wait(NULL);
        printf("child stdout left:");
        fwrite(outbuf, 1, sizeof(outbuf), stdout);
        printf("\nchild stderr left:");
        fwrite(errbuf, 1, sizeof(errbuf), stdout);
        printf("\n");
    }
    return 0;
}
