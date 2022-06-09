#if !defined(CONFIG)
#define CONFIG

#include <string_view>

using std::string_view;

constexpr char SCRIPT_ROOT_PATH[] = "/data/online_scriptor/";
constexpr char DB_PATH[] = "/data/online_scriptor/task_db";

constexpr char PYTHON_PATH[] = "/usr/bin/python3";
constexpr char BASH_PATH[] = "/usr/bin/bash";

constexpr int PORT = 6677;
constexpr int PORT_WS = 6678;

constexpr char STATIC_FILE_PREFIX[] = "/static/";
constexpr char STATIC_FILE_DIR_PATH[] = "/data/online_scriptor/static/";

#endif // CONFIG
