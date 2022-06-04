
#include "buffer_helper.hpp"
#include "cmd_msg.hpp"
#include "http_mgr.hpp"
#include "task_mgr.hpp"
#include "util.hpp"

#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>

using std::to_string, std::runtime_error;

static int cmdSocks[2] = {};
static int ioSocks[2] = {};
static BufferHelper bufferHelper;

void sigHandler(int signum) {
    if (signum == SIGINT) {
        printf("\nshutting down\n");
        for (auto &&fd : cmdSocks)
            if (fd != 0)
                close(fd);
        for (auto &&fd : ioSocks)
            if (fd != 0)
                close(fd);
        exit(0);
    }
}

CmdRes writeMsgAndWaitRes(int fd, const CmdMsg &msg) {
    BufferHelper::writeOne(fd, msg.toJson());
    return CmdRes::parse(BufferHelper::readOne(fd));
}

#if defined(TESTING)

int main(int argc, char const *argv[]) {
    signal(SIGINT, sigHandler);

    printf("initializing socks\n");
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, cmdSocks) != -1);
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, ioSocks) != -1);

    pid_t taskMgrPid = fork();
    if (taskMgrPid == -1) {
        printf("fork failed!\n");
    } else if (taskMgrPid == 0) {
        // child

        signal(SIGINT, nullptr);
        close(cmdSocks[0]), close(ioSocks[0]);
        assert(prctl(PR_SET_PDEATHSIG, SIGKILL) != -1);

        printf("starting task manager\n");
        startTaskMgr(cmdSocks[1], ioSocks[1]);

        exit(1);
    }
    // parent
    close(cmdSocks[1]), close(ioSocks[1]);
    // wait for child-ready notification
    {
        char childOK;
        assert(read(cmdSocks[0], &childOK, 1) > 0);
        // printf("__DEBUG main parent, read notification %c\n", childOK);
    }
    // TODO test here
    printf("test started\n");
    {
        int fd = cmdSocks[0];
        CmdRes res;
        // create a task
        printf("1: create\n");
        res = writeMsgAndWaitRes(fd, CmdMsg{
                                         .cmdType = CmdType::CREATE_TASK,
                                         .title = "hello world",
                                         .scriptCode = "\
import time;\n\
for i in range(10):\n\
    print('hello world: ', i, flush=True);\n\
    time.sleep(1)\n\n\
print('over')\n\
",
                                         .scriptType = TaskScriptType::PYTHON,
                                         .interval = -1,
                                         .maxTimes = 1,
                                     });
        printf("1: %s\n", res.toJsonStr().c_str());
        usleep(100000);

        printf("2: create\n");
        res = writeMsgAndWaitRes(fd, CmdMsg{
                                         .cmdType = CmdType::CREATE_TASK,
                                         .title = "hello world 2",
                                         .scriptCode = "\
import time;\n\
for i in range(10):\n\
    print('hello world: ', i, flush=True);\n\
    time.sleep(1)\n\n\
print('over')\n\
",
                                         .scriptType = TaskScriptType::PYTHON,
                                         .interval = 10 * 60,
                                         .maxTimes = 10,
                                     });
        printf("2: %s\n", res.toJsonStr().c_str());
        usleep(100000);

        printf("3: get all task\n");
        res = writeMsgAndWaitRes(fd, CmdMsg{.cmdType = CmdType::GET_ALL_TASK});
        printf("3: %s\n", res.toJsonStr().c_str());
        usleep(100000);
    }
    printf("closing socks\n");
    close(cmdSocks[0]), close(ioSocks[0]);
    // wait for unixSock to die
    wait(nullptr);
    printf("All components down, exiting.\n");
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
