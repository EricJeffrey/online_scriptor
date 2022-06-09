#if !defined(UTIL)
#define UTIL

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fmt/compile.h"
#include "fmt/core.h"

using std::string, std::runtime_error;

inline void terminateIfNot(bool expr) {
    if (!expr) {
        fmt::print("terminateIfNot: expression false!\n");
        std::terminate();
    }
}

string curTimeStr();

template <typename... Args> void println(fmt::format_string<Args...> format, const Args &...args) {
    fmt::print(format, args...);
    std::putc('\n', stdout);
}

template <typename... Args>
void printlnTime(fmt::format_string<Args...> format, const Args &...args) {
    fmt::print("[{}] ", curTimeStr());
    fmt::print(format, args...);
    std::putc('\n', stdout);
}

void writeN(int fd, const char *data, int32_t n);

void readN(int fd, char *buf, int32_t n);

// check if filePath exist and is regular file
bool filePathOk(const string &filePath);

bool endsWith(const string &s, const string &target);

#endif // UTIL
