#if !defined(UTIL)
#define UTIL

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using std::string, std::runtime_error;

inline void writeN(int fd, const char *data, int32_t n) {
    int32_t nWritten = 0;
    while (nWritten < n) {
        int32_t res = write(fd, data + nWritten, n - nWritten);
        if (res == -1 && errno != EINTR)
            throw runtime_error("writeN: write failed!" + string(strerror(errno)));
        nWritten += res;
    }
}
inline void readN(int fd, char *buf, int32_t n) {
    int32_t nRead = 0;
    while (nRead < n) {
        int res = read(fd, buf + nRead, n - nRead);
        if (res == -1 && errno == EINTR)
            continue;
        if (res == -1)
            throw runtime_error("readN: read failed! " + string(strerror(errno)));
        nRead += res;
    }
}

// check if filePath exist and is regular file
inline bool filePathOk(const string &filePath) {
    struct stat statBuf;
    bool res = true;
    if (stat(filePath.c_str(), &statBuf) == -1)
        res = false;
    if (!S_ISREG(statBuf.st_mode))
        res = false;
    return res;
}

#endif // UTIL
