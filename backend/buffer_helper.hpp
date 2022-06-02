#if !defined(BUFFER_HELPER)
#define BUFFER_HELPER

#include "nlohmann/json.hpp"
#include <optional>
#include <string>

using nlohmann::json;
using std::string, std::optional;
using ustring = std::basic_string<u_char>;

constexpr int32_t DEFAULT_BUF_SIZE = 1024;

struct BufferHelper {
    // char是有符号的，左移/右移可能产生负值
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
     * @return optional<json> 若数据完整，则返回解析到的JSON，否则返回空optional
     */
    optional<json> tryFullFill(char *buf, size_t n) {
        size_t i = 0;
        while (i < n && length.size() < 4) {
            length.push_back(buf[i]);
            ++i;
        }
        if (length.size() >= 4) {
            size_t expectedSize =
                length[0] + (length[1] << 8) + (length[2] << 16) + (length[3] << 24);
            content.reserve(expectedSize);
            while (content.size() < expectedSize && i < n) {
                content.push_back(buf[i]);
                i += 1;
            }
            if (content.size() == expectedSize) {
                json res = json::parse(content);
                length.clear();
                content.clear();
                return res;
            }
        }
        return std::nullopt;
    }
};

#endif // BUFFER_HELPER
