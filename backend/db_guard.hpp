#if !defined(DB_GUARD)
#define DB_GUARD

#include <algorithm>
#include <any>
#include <string>
#include <variant>

#include "leveldb/db.h"
#include "task.hpp"

using std::string, std::max, std::to_string;


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

    void open(bool create_if_missing = false);
    leveldb::Status writeToDB(const string &key, const string &value);
    string readDB(const string &key);
    void deleteFromDB(const string &key);
};

#endif // DB_GUARD
