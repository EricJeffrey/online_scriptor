
#include "cmd_msg.hpp"
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
    } else {
        // parent
        close(cmdSocks[1]), close(ioSocks[1]);
        // http server here
        for (size_t i = 0; i < 0; i++) {
            CmdType type = static_cast<CmdType>(rand() % (CmdType::SHUT_DOWN + 1));
            CmdMsg msg;
            switch (type) {
            case CmdType::CREATE_TASK:
                break;
            case CmdType::START_TASK:
                break;
            case CmdType::STOP_TASK:
                break;
            case CmdType::DELETE_TASK:
                break;
            case CmdType::GET_TASK:
                break;
            case CmdType::ENABLE_REDIRECT:
                break;
            case CmdType::DISABLE_REDIRECT:
                break;
            case CmdType::PUT_TO_STDIN:
                break;
            case CmdType::SHUT_DOWN:
                break;
            }
            sleep(1);
        }
        // wait task manager
        wait(nullptr);
    }
    return 0;
}

#else

int main(int argc, char const *argv[]) {
    printf("Not Implemented!\n");
    return 0;
}

#endif // TESTING
