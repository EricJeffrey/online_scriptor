#if !defined(CONFIG)
#define CONFIG

#include <string_view>

using std::string_view;

constexpr char SCRIPT_ROOT_PATH[] = "/data/online_scriptor_sjf/";
constexpr char DB_PATH[] = "/data/online_scriptor_sjf/task_db";

constexpr char PYTHON_PATH[] = "/usr/bin/python3";
constexpr char BASH_PATH[] = "/usr/bin/bash";

constexpr int PORT = 8000;

constexpr string_view STATIC_FILE_PREFIX = "/static/";
// constexpr string_view STATIC_FILE_DIR_PATH = "/data/online_scriptor_sjf/static/";
constexpr string_view STATIC_FILE_DIR_PATH = "/home/sjf/coding/online_scriptor/frontend/";

#endif // CONFIG
