#if !defined(TASK_DB_HELPER)
#define TASK_DB_HELPER

#include "task.hpp"
#include <string>
#include <vector>

using std::string, std::vector;

// call TaskDBHelper::init before use
struct TaskDBHelper {
    static void init();
    static int32_t createTask(const string &title, const string &scriptCode, int32_t scriptType, int32_t intervalInSec=0, int32_t maxTimes=1);
    static Task getTask(int32_t taskId);
    static void deleteTask(int32_t taskId);
    static void updateTask(int32_t taskId, const Task &newTask);
    static bool hasTask(int32_t taskId);
    static vector<Task> getAllTask();
};

#endif // TASK_DB_HELPER
