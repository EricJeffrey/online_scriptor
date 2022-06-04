#include "db_guard.hpp"
#include "config.hpp"

void DBGuard::open(bool create_if_missing) {
    leveldb::Options options;
    options.create_if_missing = create_if_missing;
    if (!leveldb::DB::Open(options, DB_PATH, &db).ok())
        throw std::runtime_error("open leveldb failed");
    opened = true;
}

leveldb::Status DBGuard::writeToDB(const string &key, const string &value) {
    checkOpened();
    auto status = db->Put(leveldb::WriteOptions(), key, value);
    if (!status.ok())
        throw std::runtime_error("db write failed: " + status.ToString());
    return status;
}

string DBGuard::readDB(const string &key) {
    checkOpened();
    string value;
    auto status = db->Get(leveldb::ReadOptions(), key, &value);
    if (!status.ok())
        throw std::runtime_error("db read failed: " + status.ToString());
    return value;
}

void DBGuard::deleteFromDB(const string &key) {
    checkOpened();
    auto status = db->Delete(leveldb::WriteOptions(), key);
    if (!status.ok())
        throw std::runtime_error("db delete key failed: " + status.ToString());
}
