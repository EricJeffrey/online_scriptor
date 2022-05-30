#if !defined(TASK_DB_MGR)
#define TASK_DB_MGR

#include <algorithm>
#include <any>
#include <string>
#include <variant>

#include "leveldb/db.h"

#include "task.hpp"

namespace task_db_mgr {

    using std::string, std::max, std::to_string;

    using task::Task;

    constexpr char *DB_PATH = "/data/online_scriptor/task_db";
    constexpr char *DB_KEY_TASK_ID_SET = "taskIdSet";

    // RAII Guard of leveldb::DB, call DBGuard::init before use!
    struct DBGuard {
        leveldb::DB *db;
        bool opened;

        DBGuard() : db(nullptr), opened(false){};
        ~DBGuard() { delete db; }

        static void init();

        inline void checkOpened() {
            if (!opened)
                throw std::runtime_error("db is not opened");
        }

        void open(bool create_if_missing = false) {
            leveldb::Options options;
            options.create_if_missing = create_if_missing;
            if (!leveldb::DB::Open(options, DB_PATH, &db).ok())
                throw std::runtime_error("open leveldb failed");
            opened = true;
        }

        leveldb::Status writeToDB(const string &key, const string &value) {
            checkOpened();
            auto status = db->Put(leveldb::WriteOptions(), key, value);
            if (!status.ok())
                throw std::runtime_error("db write failed: " + status.ToString());
        }

        string readDB(const string &key) {
            checkOpened();
            string value;
            auto status = db->Get(leveldb::ReadOptions(), key, &value);
            if (!status.ok())
                throw std::runtime_error("db read failed: " + status.ToString());
            return value;
        }

        void deleteFromDB(const string &key) {
            checkOpened();
            auto status = db->Delete(leveldb::WriteOptions(), key);
            if (!status.ok())
                throw std::runtime_error("db delete key failed: " + status.ToString());
        }
    };

    void createTask(const string &title, const string &scriptCode, int32_t scriptType,
                    int32_t intervalInSec = 0, int32_t maxTimes = 1);
    void deleteTask(int32_t taskId);
    void updateTask(int32_t taskId, const Task &newTask);

} // namespace task_db_mgr

#endif // TASK_DB_MGR
