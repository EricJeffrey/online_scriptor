
#include "cmd_msg.hpp"
#include "http_mgr.hpp"
#include "task_mgr.hpp"

#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>

using std::to_string;

#if defined(TESTING)

int main(int argc, char const *argv[]) {
    int cmdSocks[2], ioSocks[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, cmdSocks) != -1);
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, ioSocks) != -1);
    pid_t taskMgrPid = fork();
    if (taskMgrPid == -1) {
        printf("fork failed!\n");
    } else if (taskMgrPid == 0) {
        // child
        close(cmdSocks[0]), close(ioSocks[0]);
        assert(prctl(PR_SET_PDEATHSIG, SIGKILL) != -1);
        startTaskMgr(cmdSocks[1], ioSocks[1]);
        exit(1);
    }
    // parent
    close(cmdSocks[1]), close(ioSocks[1]);
    close(cmdSocks[0]), close(ioSocks[0]);
    // TODO test here

    // wait for unixSock to die
    wait(nullptr);
    return 0;
}

#else

int main(int argc, char const *argv[]) {
    int cmdSocks[2], ioSocks[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, cmdSocks) != -1);
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, ioSocks) != -1);
    pid_t taskMgrPid = fork();
    if (taskMgrPid == -1) {
        printf("fork failed!\n");
    } else if (taskMgrPid == 0) {
        // child
        close(cmdSocks[0]), close(ioSocks[0]);
        assert(prctl(PR_SET_PDEATHSIG, SIGKILL) != -1);
        startTaskMgr(cmdSocks[1], ioSocks[1]);
        exit(1);
    }
    // parent
    close(cmdSocks[1]), close(ioSocks[1]);
    HttpMgr::start(cmdSocks[0], ioSocks[0]);

    // wait for unixSock to die
    wait(nullptr);
    return 0;
}

#endif // TESTING
