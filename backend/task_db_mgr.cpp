#include <algorithm>
#include <any>
#include <string>
#include <variant>

#include "leveldb/db.h"

#include "task.hpp"

using std::string, std::max;

using task::Task;

struct DBGuard {
    leveldb::DB *db;
    bool opened;

    DBGuard() : db(nullptr), opened(false){};
    ~DBGuard() { delete db; }

    static void init();

    void open(bool create_if_missing = false) {
        static const string DB_PATH = "/data/online_scriptor/task_db";
        leveldb::Options options;
        options.create_if_missing = create_if_missing;
        if (!leveldb::DB::Open(options, DB_PATH, &db).ok())
            throw std::runtime_error("open leveldb failed");
        opened = true;
    }

    leveldb::Status writeToDB(const string &key, const string &value) {
        if (!opened)
            throw std::runtime_error("db is not opened");
        return db->Put(leveldb::WriteOptions(), key, value);
    }

    string readDB(const string &key) {
        if (!opened)
            throw std::runtime_error("db is not opened");
        string value;
        auto status = db->Get(leveldb::ReadOptions(), key, &value);
        if (!status.ok())
            throw std::runtime_error("db read failed");
        return value;
    }
};

void DBGuard::init() { DBGuard().open(true); }

int32_t getNextTaskId(DBGuard &guard) {
    static const string DB_KEY_TASK_ID_SET = "taskIdSet";

    auto idListJson = json::parse(guard.readDB(DB_KEY_TASK_ID_SET));
    int32_t maxId = 0;
    if (!idListJson.empty()) {
        for (auto &&v : idListJson) {
            maxId = max(maxId, v.get<int32_t>());
        }
    }
    return maxId + 1;
}

void createTask(const string &title, const string &scriptCode, int32_t scriptType,
                int32_t intervalInSec = 0, int32_t maxTimes = 1) {
    Task newTask;
    newTask.title = title;
    newTask.scriptCode = scriptCode;
    newTask.scriptType = static_cast<task::ScriptType>(scriptType);
    newTask.status = task::IDLE;
    newTask.pid = 0;
    newTask.intervalInSec = intervalInSec;
    newTask.maxTimes = maxTimes;
    newTask.timesExecuted = 0;
    newTask.exitCode = 0;
    newTask.exitTimeStamp = 0;

    DBGuard guard;
    guard.open();
    newTask.id = getNextTaskId(guard);
    guard.writeToDB(std::to_string(newTask.id), newTask.toJson().dump());
}