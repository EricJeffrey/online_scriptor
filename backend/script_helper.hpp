#if !defined(SCRIPT_HELPER)
#define SCRIPT_HELPER

#include <sys/types.h>
#include <unistd.h>

#include <stdexcept>
#include <string>
#include <vector>

using std::string, std::runtime_error, std::vector;

struct StartScriptRes {
    int fdStdin, fdStdout, fdStderr;
    pid_t childPid;
};

StartScriptRes startPyScriptWithIORedir(const string &filePath, const vector<int> &fdsToClose);

StartScriptRes startBashScriptWithIORedir(const string &filePath, const vector<int> &fdsToClose);

#endif // SCRIPT_HELPER
