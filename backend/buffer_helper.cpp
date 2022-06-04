#include "buffer_helper.hpp"
#include "util.hpp"

vector<json> BufferHelper::tryFullFill(char *buf, size_t n) {
    // return (full, iUsed)
    auto fullFillOne = [this](char *tmpBuf, size_t bufSize) {
        size_t i = 0;
        while (i < bufSize && length.size() < 4) {
            length.push_back(tmpBuf[i]);
            ++i;
        }
        if (length.size() == 4) {
            size_t expectedSize =
                length[0] + (length[1] << 8) + (length[2] << 16) + (length[3] << 24);
            while (content.size() < expectedSize && i < bufSize) {
                content.push_back(tmpBuf[i]);
                i += 1;
            }
            return std::make_pair(content.size() == expectedSize, i);
        }
        return std::make_pair(false, i);
    };
    vector<json> resJsonList;
    size_t i = 0;
    while (i < n) {
        auto tmpRes = fullFillOne(buf + i, n - i);
        if (tmpRes.first) {
            resJsonList.push_back(json::parse(std::move(content)));
            length.clear();
            content.clear();
        }
        i += tmpRes.second;
    }
    return resJsonList;
}

json BufferHelper::readOne(int fd) {

    char lenBuf[4];
    readN(fd, lenBuf, 4);

    int32_t bodyLen = 0;
    for (int i = 0; i < 4; i++)
        bodyLen += (((u_char)lenBuf[i]) << (i * 8));
    string bodyBuf(bodyLen, 0);
    readN(fd, bodyBuf.data(), bodyLen);
    return json::parse(bodyBuf);
}

string BufferHelper::make(string &&jsonData) {
    size_t n = jsonData.size();
    string data({
        static_cast<char>(u_char(n % 256)),
        static_cast<char>(u_char((n >> 8) % 256)),
        static_cast<char>(u_char((n >> 16) % 256)),
        static_cast<char>(u_char((n >> 24) % 256)),
    });
    data.append(jsonData);
    return data;
}

void BufferHelper::writeOne(int fd, const json &data) {
    const string &s = make(data.dump());
    writeN(fd, s.data(), s.size());
}