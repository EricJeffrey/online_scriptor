
#include "task_db_helper.hpp"
#include "db_guard.hpp"

constexpr char DB_KEY_TASK_ID_SET[] = "taskIdSet";

using std::string, std::max, std::to_string;

void TaskDBHelper::init() {
    DBGuard guard;
    guard.open(true);
    guard.writeToDB(DB_KEY_TASK_ID_SET, "[]");
}

bool TaskDBHelper::hasTask(int32_t taskId) {
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
    return found;
}

int32_t TaskDBHelper::createTask(const string &title, const string &scriptCode, int32_t scriptType,
                                 int32_t intervalInSec, int32_t maxTimes) {
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
    return newTask.id;
}

Task TaskDBHelper::getTask(int32_t taskId) {
    if (hasTask(taskId)) {
        DBGuard guard;
        guard.open();
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

vector<Task> TaskDBHelper::getAllTask() {
    json idListJson;
    {
        DBGuard guard;
        guard.open();
        idListJson = json::parse(guard.readDB(DB_KEY_TASK_ID_SET));
    }
    vector<Task> res;
    res.reserve(idListJson.size());
    for (auto &&id : idListJson) {
        res.push_back(getTask(id.get<int32_t>()));
    }
    return res;
}