#include "script_helper.hpp"
#include "config.hpp"
#include "util.hpp"

#include <sys/prctl.h>
#include <sys/stat.h>

#include <csignal>
#include <cstring>

const int32_t INTERPRETOR_TYPE_PYTHON = 1;
const int32_t INTERPRETOR_TYPE_BASH = 2;

/**
 * @brief 使用 type 指定的解释器执行 filePath 的脚本，返回重定向的输入和输出管道
 *
 * @param filePath 脚本路径
 * @param type 脚本类型
 * @param preWork 进程创建后需要预先执行的函数
 * @return StartScriptRes 返回结果，包括重定向后的三个fd和子进程的pid
 */
static StartScriptRes startScriptWithIORedir(const string &filePath, int32_t type, const vector<int> &fdsToClose) {
    if (filePath.empty() || !filePathOk(filePath)) {
        throw runtime_error("startScriptWithIORedir: invalid filePath or type");
    }
    int pipeStdout[2], pipeStderr[2], pipeStdin[2];
    if (pipe(pipeStdin) == -1 || pipe(pipeStdout) == -1 || pipe(pipeStderr) == -1) {
        throw runtime_error("pipe failed!");
    }
    pid_t childPid = fork();
    if (childPid == -1) {
        throw runtime_error("fork failed!");
    } else if (childPid == 0) {
        for (auto &&fd : fdsToClose) {
            close(fd);
        }

        close(pipeStdin[1]), close(pipeStdout[0]), close(pipeStderr[0]);
        // redirect IO
        if (dup2(pipeStdin[0], 0) == -1 || dup2(pipeStdout[1], 1) == -1 ||
            dup2(pipeStderr[1], 2) == -1)
            throw runtime_error("child script, dup2 failed!");
        // make sure child got killed when parent dead
        if (prctl(PR_SET_PDEATHSIG, SIGKILL))
            throw runtime_error("child script, prctl failed!");
        // tell parent we are done
        if (write(pipeStdout[1], "1", 1) == -1)
            throw runtime_error("child script, write failed!");

        if (type == INTERPRETOR_TYPE_PYTHON)
            execl(PYTHON_PATH, "python3", filePath.c_str(), nullptr);
        if (type == INTERPRETOR_TYPE_BASH)
            execl(BASH_PATH, "bash", filePath.c_str(), nullptr);
        throw runtime_error("child script, execl failed");
    } else {
        close(pipeStdin[0]), close(pipeStdout[1]), close(pipeStderr[1]);
        char ch;
        if (read(pipeStdout[0], &ch, 1) == -1)
            throw runtime_error("startScriptWithIORedir error: child failed to start!");
        return StartScriptRes{.fdStdin = pipeStdin[1],
                              .fdStdout = pipeStdout[0],
                              .fdStderr = pipeStderr[0],
                              .childPid = childPid};
    }
}

/**
 * @brief 使用Python解释器执行 filePath 的脚本，返回重定向的输入和输出管道
 *
 * @param filePath 脚本路径
 * @param preWork 进程创建后需要预先执行的函数
 * @return StartScriptRes 返回结果，包括重定向后的三个fd和子进程的pid
 */
StartScriptRes ScriptHelper::startPyScriptWithIORedir(const string &filePath, const vector<int> &fdsToClose) {
    return startScriptWithIORedir(filePath, INTERPRETOR_TYPE_PYTHON, fdsToClose);
}

/**
 * @brief 使用Bash解释器执行 filePath 的脚本，返回重定向的输入和输出管道
 *
 * @param filePath 脚本路径
 * @param preWork 进程创建后需要预先执行的函数
 * @return StartScriptRes 返回结果，包括重定向后的三个fd和子进程的pid
 */
StartScriptRes ScriptHelper::startBashScriptWithIORedir(const string &filePath, const vector<int> &fdsToClose) {
    return startScriptWithIORedir(filePath, INTERPRETOR_TYPE_BASH, fdsToClose);
}
