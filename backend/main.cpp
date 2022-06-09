#include "config.hpp"
#include "http_mgr.hpp"
#include "task_mgr.hpp"
#include "util.hpp"

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
        println("\nShutting down");
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

    println("Configuration:");
    println("\tScript Root Path: {}", SCRIPT_ROOT_PATH);
    println("\tDatabase Path: {}", DB_PATH);
    println("\tPython Interpreter Path: {}", PYTHON_PATH);
    println("\tBash Interpreter Path: {}", BASH_PATH);
    println("\tHttp Listen Port: {}", PORT);
    println("\tWebsocket Listen Port: {}", PORT_WS);
    println("\tStatic File Path: {}", STATIC_FILE_DIR_PATH);
    println("\tStatic File Url Prefix: {}", STATIC_FILE_PREFIX);
    println("------------------------------");

    println("Initializing socks");
    terminateIfNot(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, cmdSocks) != -1);
    terminateIfNot(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, ioSocks) != -1);

    pid_t taskMgrPid = fork();
    terminateIfNot(taskMgrPid != -1);
    // child
    if (taskMgrPid == 0) {

        signal(SIGINT, nullptr);
        close(cmdSocks[0]), close(ioSocks[0]);
        terminateIfNot(prctl(PR_SET_PDEATHSIG, SIGKILL) != -1);

        println("Starting task manager");
        startTaskMgr(cmdSocks[1], ioSocks[1]);

        exit(1);
    }
    // parent
    close(cmdSocks[1]), close(ioSocks[1]);

    // wait for child-ready notification
    char childReadyByte = 0;
    terminateIfNot(read(cmdSocks[0], &childReadyByte, 1) > 0);

    fmt::print("__DEBUG childPid: {}, press any key to continue.", taskMgrPid);
    getchar();

    println("Starting http/websocket manager");
    startHttpWsMgr(cmdSocks[0], ioSocks[0]);

    close(cmdSocks[0]), close(ioSocks[0]);
    wait(nullptr);
    println("All components down, exiting.");
    return 0;
}
