#if !defined(TASK_DB_HELPER)
#define TASK_DB_HELPER

#include "task.hpp"
#include <string>

using std::string;

// call TaskDBHelper::init before use
struct TaskDBHelper {
    static void init();
    static void createTask(const string &title, const string &scriptCode, int32_t scriptType, int32_t intervalInSec, int32_t maxTimes);
    static Task getTask(int32_t taskId);
    static void deleteTask(int32_t taskId);
    static void updateTask(int32_t taskId, const Task &newTask);
};

#endif // TASK_DB_HELPER
