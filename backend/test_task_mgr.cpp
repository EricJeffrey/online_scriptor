#include "test_task_mgr.hpp"
#include "buffer_helper.hpp"
#include "cmd_msg.hpp"
#include "task_mgr.hpp"
#include "util.hpp"

#include "fmt/color.h"
#include "fmt/format.h"

#include "sys/wait.h"
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cassert>
#include <csignal>
#include <thread>

using std::to_string, std::runtime_error, std::thread;

static int cmdSocks[2] = {};
static int ioSocks[2] = {};

void _sigHandler(int signum) {
    if (signum == SIGINT) {
        fmt::print("\nshutting down\n");
        for (auto &&fd : cmdSocks)
            if (fd != 0)
                close(fd);
        for (auto &&fd : ioSocks)
            if (fd != 0)
                close(fd);
        exit(0);
    }
}

void testTaskMgr() {
    signal(SIGINT, _sigHandler);

    fmt::print("initializing socks\n");
    terminateIfNot(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, cmdSocks) != -1);
    terminateIfNot(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, ioSocks) != -1);

    pid_t taskMgrPid = fork();
    if (taskMgrPid == -1) {
        fmt::print("fork failed!\n");
    } else if (taskMgrPid == 0) {
        // child

        signal(SIGINT, nullptr);
        close(cmdSocks[0]), close(ioSocks[0]);
        terminateIfNot(prctl(PR_SET_PDEATHSIG, SIGKILL) != -1);

        fmt::print("starting task manager\n");
        startTaskMgr(cmdSocks[1], ioSocks[1]);

        exit(1);
    }
    // parent
    close(cmdSocks[1]), close(ioSocks[1]);
    // wait for child-ready notification
    {
        char childOK;
        terminateIfNot(read(cmdSocks[0], &childOK, 1) > 0);
    }
    fmt::print("__DEBUG pid of TaskMgr is {}. Attch GDB and press enter to continue", taskMgrPid);
    getchar();

    auto testOne = [](const CmdMsg &msg, CmdRes::Type resStatus = CmdRes::Type::OK) {
        static int index = 1;
        fmt::print("----------------------------------------\n");
        fmt::print("\n{}, {}\n", index, CMDTYPE_TO_STRING_VIEW[msg.cmdType].data());
        BufferHelper::writeOne(cmdSocks[0], msg.toJson());
        CmdRes res = CmdRes::parse(BufferHelper::readOne(cmdSocks[0]));
        fmt::print("res.status: {}\n\n", CMDRESTYPE_TO_STRING_VIEW[res.status]);
        fmt::print("res: {}\n\n", res.toJsonStr().c_str());
        if (resStatus == res.status)
            fmt::print(fmt::fg(fmt::color::light_green), "PASSED\n");
        else
            fmt::print(fmt::fg(fmt::color::red), "FAILED\n");
        usleep(100000);
        index += 1;
    };

    // testing
    {
        fmt::print("---------- test started ----------\n");

        testOne(CmdMsg{.cmdType = CmdMsg::Type::GET_ALL_TASK}, CmdRes::Type::OK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::GET_TASK, .taskId = 10},
                CmdRes::Type::NO_SUCH_TASK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::START_TASK, .taskId = 10},
                CmdRes::Type::NO_SUCH_TASK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::DELETE_TASK, .taskId = 10},
                CmdRes::Type::NO_SUCH_TASK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::STOP_TASK, .taskId = 10},
                CmdRes::Type::NO_SUCH_TASK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::CREATE_TASK,
                       .title = "hello world",
                       .scriptCode =
                           "import time;\nfor i in range(20):\n    print('hello world: ', i, "
                           "flush=True);\n    time.sleep(1)\n\nprint('over')\n",
                       .scriptType = TaskScriptType::PYTHON,
                       .interval = -1,
                       .maxTimes = 1},
                CmdRes::Type::OK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::CREATE_TASK,
                       .title = "hello world 2",
                       .scriptCode =
                           "import time;\nfor i in range(10):\n    print('hello world: ', i, "
                           "flush=True);\n    time.sleep(1)\n\nprint('over')\n",
                       .scriptType = TaskScriptType::PYTHON,
                       .interval = 10 * 60,
                       .maxTimes = 10},
                CmdRes::Type::OK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::GET_ALL_TASK}, CmdRes::Type::OK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::GET_TASK, .taskId = 10},
                CmdRes::Type::NO_SUCH_TASK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::GET_TASK, .taskId = 1}, CmdRes::Type::OK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::START_TASK, .taskId = 10},
                CmdRes::Type::NO_SUCH_TASK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::ENABLE_REDIRECT, .taskId = 10},
                CmdRes::Type::TASK_NOT_RUNNING);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::ENABLE_REDIRECT, .taskId = 2},
                CmdRes::Type::TASK_NOT_RUNNING);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::DISABLE_REDIRECT, .taskId = 1},
                CmdRes::Type::TASK_NOT_RUNNING);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::START_TASK, .taskId = 1}, CmdRes::Type::OK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::ENABLE_REDIRECT, .taskId = 1}, CmdRes::Type::OK);

        // it will block untile program die, just ignore it
        thread([]() {
            while (true) {
                fmt::print("IOSock: {}\n", BufferHelper::readOne(ioSocks[0]).dump());
            }
        }).detach();

        sleep(3);
        testOne(CmdMsg{.cmdType = CmdMsg::Type::DISABLE_REDIRECT, .taskId = 1});

        testOne(CmdMsg{.cmdType = CmdMsg::Type::CREATE_TASK,
                       .title = "echo world",
                       .scriptCode = "\
import time\n\
for i in range(10):\n\
    print('lets see somthing first', flush=True)\n\
    x = input(\"say something:\")\n\
    print(\"you say {}\".format(x), flush=True)\n\
    time.sleep(1)\n\
print(\"over\")\n\
",
                       .scriptType = TaskScriptType::PYTHON,
                       .interval = 10,
                       .maxTimes = 1},
                CmdRes::Type::OK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::START_TASK, .taskId = 30},
                CmdRes::Type::NO_SUCH_TASK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::START_TASK, .taskId = 3});

        testOne(CmdMsg{.cmdType = CmdMsg::Type::ENABLE_REDIRECT, .taskId = 3});

        sleep(2);
        testOne(CmdMsg{
            .cmdType = CmdMsg::Type::PUT_TO_STDIN, .taskId = 3, .stdinContent = "rainnnnnnnnn\n"});

        testOne(CmdMsg{.cmdType = CmdMsg::Type::CREATE_TASK,
                       .title = "hello world 2",
                       .scriptCode = "import time;\nfor i in range(20):\n    print('!!=_=!! ', i, "
                                     "flush=True);\n    time.sleep(1)\n\nprint('over')\n",
                       .scriptType = TaskScriptType::PYTHON,
                       .interval = 10,
                       .maxTimes = 10},
                CmdRes::Type::OK);

        testOne(CmdMsg{.cmdType = CmdMsg::Type::START_TASK, .taskId = 4});

        fmt::print("waiting for 31 sec, STAY CALM\n");
        sleep(31);
        testOne(CmdMsg{.cmdType = CmdMsg::Type::ENABLE_REDIRECT, .taskId = 4});

        sleep(1);
        testOne(CmdMsg{.cmdType = CmdMsg::Type::DISABLE_REDIRECT, .taskId = 4});

        sleep(1);
        testOne(CmdMsg{.cmdType = CmdMsg::Type::ENABLE_REDIRECT, .taskId = 4});

        sleep(1);
        testOne(CmdMsg{.cmdType = CmdMsg::Type::DELETE_TASK, .taskId = 4});

        testOne(CmdMsg{.cmdType = CmdMsg::Type::STOP_TASK, .taskId = 4});

        fmt::print("---------- test done ----------\n");
    }
    close(cmdSocks[0]), close(ioSocks[0]);
    // wait for unixSock to die
    wait(nullptr);
    fmt::print("All components down, exiting.\n");
    return;
}
