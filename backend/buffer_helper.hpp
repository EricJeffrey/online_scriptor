#if !defined(BUFFER_HELPER)
#define BUFFER_HELPER

#include "nlohmann/json.hpp"
#include <optional>
#include <string>
#include <vector>

using nlohmann::json;
using std::string, std::optional, std::vector;
using ustring = std::basic_string<u_char>;

constexpr int32_t DEFAULT_BUF_SIZE = 1024;

/**
 * @brief 长度+字符串的缓冲区工具类，用来得到指定长度的数据并解析为JSON字符串
 *
 */
struct BufferHelper {
    // char是有符号的，左移/右移可能产生负值，因此使用u_char
    ustring length;
    string content;

    BufferHelper() {
        length.reserve(4);
        content.reserve(DEFAULT_BUF_SIZE);
    }

    /**
     * @brief 存储数据，并在获取完整长度后解析为JSON返回
     *
     * @param buf 缓冲区
     * @param n 大小
     * @return optional<json> 若数据完整，则返回解析到的多个JSON，并清空内部存储，否则返回空optional
     */
    vector<json> tryFullFill(char *buf, size_t n) {
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
            }
            i += tmpRes.second;
        }
        return resJsonList;
    }

    inline static string make(string &&jsonData) {
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
};

#endif // BUFFER_HELPER
