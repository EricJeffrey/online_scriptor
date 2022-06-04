#if !defined(BUFFER_HELPER)
#define BUFFER_HELPER

#include "nlohmann/json.hpp"
#include <string>
#include <vector>

using nlohmann::json;
using std::string, std::vector;
using ustring = std::basic_string<u_char>;

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
        content.reserve(2048);
    }

    /**
     * @brief 存储数据，并在获取完整长度后解析为JSON返回
     *
     * @param buf 缓冲区
     * @param n 大小
     * @return optional<json> 若数据完整，则返回解析到的多个JSON，并清空内部存储，否则返回空optional
     */
    vector<json> tryFullFill(char *buf, size_t n);

    /**
     * @brief 从fd读取一个 长度+JSON字符串 的数据，阻塞直到获取完整数据
     *
     * @param fd 文件描述符
     * @return json 解析到的JSON数据
     */
    static json readOne(int fd);

    static void writeOne(int fd, const json&data);

    static string make(string &&jsonData);
};

#endif // BUFFER_HELPER
