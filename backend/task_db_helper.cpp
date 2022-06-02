
#include "task_db_helper.hpp"
#include "db_guard.hpp"

constexpr char *DB_KEY_TASK_ID_SET = "taskIdSet";
using std::string, std::max, std::to_string;

static bool isInDB(DBGuard &guard, int32_t taskId) {
    auto idListJson = json::parse(guard.readDB(DB_KEY_TASK_ID_SET));
    bool found = false;
    for (size_t i = 0; i < idListJson.size(); i++) {
        if (idListJson[i].get<int32_t>() == taskId) {
            idListJson.erase(i);
            found = true;
            break;
        }
    }
    return found;
}

void TaskDBHelper::createTask(const string &title, const string &scriptCode, int32_t scriptType,
                              int32_t intervalInSec = 0, int32_t maxTimes = 1) {
    Task newTask;
    newTask.title = title;
    newTask.scriptCode = scriptCode;
    newTask.scriptType = static_cast<TaskScriptType>(scriptType);
    newTask.status = TaskStatus::IDLE;
    newTask.pid = 0;
    newTask.intervalInSec = intervalInSec;
    newTask.maxTimes = maxTimes;
    newTask.timesExecuted = 0;
    newTask.exitCode = 0;
    newTask.exitTimeStamp = 0;

    DBGuard guard;
    guard.open();
    auto idListJson = json::parse(guard.readDB(DB_KEY_TASK_ID_SET));
    int32_t maxId = 0;
    for (auto &&v : idListJson) {
        maxId = max(maxId, v.get<int32_t>());
    }
    newTask.id = maxId + 1;
    idListJson.push_back(newTask.id);
    guard.writeToDB(DB_KEY_TASK_ID_SET, idListJson.dump());
    guard.writeToDB(to_string(newTask.id), newTask.toJson().dump());
}

Task TaskDBHelper::getTask(int32_t taskId) {
    DBGuard guard;
    guard.open();
    if (isInDB(guard, taskId)) {
        return Task(json::parse(guard.readDB(to_string(taskId))));
    } else {
        throw std::runtime_error("get task failed, task not found");
    }
}

void TaskDBHelper::deleteTask(int32_t taskId) {
    DBGuard guard;
    guard.open();
    auto idListJson = json::parse(guard.readDB(DB_KEY_TASK_ID_SET));
    bool found = false;
    for (size_t i = 0; i < idListJson.size(); i++) {
        if (idListJson[i].get<int32_t>() == taskId) {
            idListJson.erase(i);
            found = true;
            break;
        }
    }
    if (!found)
        throw std::runtime_error("db delete failed: task " + to_string(taskId) + " not exist");
    guard.writeToDB(DB_KEY_TASK_ID_SET, idListJson.dump());
}

void TaskDBHelper::updateTask(int32_t taskId, const Task &newTask) {
    DBGuard guard;
    guard.open();
    auto idListJson = json::parse(guard.readDB(DB_KEY_TASK_ID_SET));
    bool found = false;
    for (size_t i = 0; i < idListJson.size(); i++) {
        if (idListJson[i].get<int32_t>() == taskId) {
            idListJson.erase(i);
            found = true;
            break;
        }
    }
    if (!found)
        throw std::runtime_error("db delete failed: task " + to_string(taskId) + " not exist");
    guard.writeToDB(to_string(taskId), newTask.toJson().dump());
}
