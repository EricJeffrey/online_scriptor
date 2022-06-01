#if !defined(TASK_DB_HELPER)
#define TASK_DB_HELPER

#include "task.hpp"
#include <string>

using std::string;

void createTask(const string &title, const string &scriptCode, int32_t scriptType,
                int32_t intervalInSec = 0, int32_t maxTimes = 1);
void deleteTask(int32_t taskId);
void updateTask(int32_t taskId, const Task &newTask);

#endif // TASK_DB_HELPER
