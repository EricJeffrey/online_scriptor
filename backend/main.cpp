#include "http_mgr.hpp"
#include "task_mgr.hpp"
#include "test_task_mgr.hpp"

#include "fmt/format.h"

#include <cassert>
#include <csignal>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

static int cmdSocks[2] = {};
static int ioSocks[2] = {};

void sigHandler(int signum) {
    if (signum == SIGINT) {
        fmt::print("\nShutting down\n");
        for (auto &&fd : cmdSocks)
            if (fd != 0)
                close(fd);
        for (auto &&fd : ioSocks)
            if (fd != 0)
                close(fd);
        exit(0);
    }
}

int main(int argc, char const *argv[]) {
    signal(SIGINT, sigHandler);

    fmt::print("Initializing socks\n");
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, cmdSocks) != -1);
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, ioSocks) != -1);

    pid_t taskMgrPid = fork();
    assert(taskMgrPid != -1);
    // child
    if (taskMgrPid == 0) {

        signal(SIGINT, nullptr);
        close(cmdSocks[0]), close(ioSocks[0]);
        assert(prctl(PR_SET_PDEATHSIG, SIGKILL) != -1);

        fmt::print("Starting task manager\n");
        startTaskMgr(cmdSocks[1], ioSocks[1]);

        exit(1);
    }
    // parent
    close(cmdSocks[1]), close(ioSocks[1]);

    // wait for child-ready notification
    char childReadyByte = 0;
    assert(read(cmdSocks[0], &childReadyByte, 1) > 0);

    fmt::print("__DEBUG childPid: {}, press any key to continue.", taskMgrPid);
    getchar();

    fmt::print("Starting http manager\n");
    HttpMgr::start(cmdSocks[0], ioSocks[0]);

    close(cmdSocks[0]), close(ioSocks[0]);
    wait(nullptr);
    fmt::print("All components down, exiting.\n");
    return 0;
}
