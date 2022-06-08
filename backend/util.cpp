#include "util.hpp"

void writeN(int fd, const char *data, int32_t n) {
    ssize_t nWritten = 0;
    while (nWritten < n) {
        ssize_t res = write(fd, data + nWritten, n - nWritten);
        if (res == -1 && errno != EINTR)
            throw runtime_error("writeN: write failed!" + string(strerror(errno)));
        nWritten += res;
    }
}
void readN(int fd, char *buf, int32_t n) {
    ssize_t nRead = 0;
    while (nRead < n) {
        ssize_t res = read(fd, buf + nRead, n - nRead);
        if (res == -1 && errno == EINTR)
            continue;
        if (res == -1)
            throw runtime_error("readN: read failed! " + string(strerror(errno)));
        nRead += res;
    }
}

// check if filePath exist and is regular file
bool filePathOk(const string &filePath) {
    struct stat statBuf;
    bool res = true;
    if (stat(filePath.c_str(), &statBuf) == -1)
        res = false;
    if (!S_ISREG(statBuf.st_mode))
        res = false;
    return res;
}

bool endsWith(const string &s, const string &target) {
    bool res = true;
    if (s.size() >= target.size()) {
        for (size_t i = 0; i < target.size(); i++)
            if (s[s.size() - target.size() + i] != target[i]) {
                res = false;
                break;
            }
    } else {
        res = false;
    }
    return res;
}

string curTimeStr() {
    std::time_t currenttime = std::time(0);
    char tAll[255] = {};
    std::strftime(tAll, sizeof(tAll), "%Y-%m-%d %H:%M:%S", std::localtime(&currenttime));
    return tAll;
}
