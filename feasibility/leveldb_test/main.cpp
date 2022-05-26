#include "leveldb/db.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

struct Guard {
    leveldb::DB *db;
    Guard() : db(nullptr){};
    ~Guard() { delete db; }
};

void childJob(const char *dbpath, int pfd) {
    char ch;
    read(pfd, &ch, 1);
    printf("child started\n");
    for (int i = 0; i < 10; i++) {
        {
            Guard guard;
            leveldb::Options options;
            if (!leveldb::DB::Open(options, dbpath, &guard.db).ok()) {
                fprintf(stderr, "child, open failed\n");
                break;
            }
            const string key = "hello" + to_string(i);
            string value;
            if (!guard.db->Get(leveldb::ReadOptions(), key, &value).ok()) {
                fprintf(stderr, "child, db get failed\n");
                break;
            }
            nlohmann::json j = nlohmann::json::parse(move(value));
            printf("child, got %s\n", j.dump().c_str());
            j["index"] = j["index"].get<int>() * 10;
            guard.db->Put(leveldb::WriteOptions(), "hello" + to_string(i), j.dump());
        }
        sleep(3);
    }
    close(pfd);
}

int main(int argc, char const *argv[]) {
    int pfds[2];
    const char DB_PATH[] = "./test.db.o";
    pipe(pfds);
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork failed\n");
        return 1;
    } else if (pid == 0) {
        close(pfds[1]);
        childJob(DB_PATH, pfds[0]);
        exit(0);
    } else {
        close(pfds[0]);
        printf("parent started\n");
        for (int i = 0; i < 10; i++) {
            {
                bool bad = false;
                Guard guard;
                leveldb::Options options;
                options.create_if_missing = true;
                while (true) {
                    auto status = leveldb::DB::Open(options, DB_PATH, &guard.db);
                    if (status.ok())
                        break;
                    if (!status.ok() && !status.IsIOError()) {
                        fprintf(stderr, "create leveldb failed: %s\n", status.ToString().c_str());
                        bad = true;
                        break;
                    }
                    sleep(1);
                }
                if (bad)
                    break;
                if (i == 0)
                    write(pfds[1], "1", 1);
                nlohmann::json value = {{"index", i}};
                if (!guard.db->Put(leveldb::WriteOptions(), "hello" + to_string(i), value.dump()).ok()) {
                    fprintf(stderr, "write to db failed\n");
                    break;
                }
                if (i != 0) {
                    const string lastkey = "hello" + to_string(i - 1);
                    string tmp;
                    if (!guard.db->Get(leveldb::ReadOptions(), lastkey, &tmp).ok()) {
                        fprintf(stderr, "get db failed\n");
                        break;
                    }
                    nlohmann::json lastv = nlohmann::json::parse(move(tmp));
                    printf("last i: %d, last key: %s, value: %s\n", i - 1, lastkey.c_str(), lastv.dump().c_str());
                }
            }
            sleep(2);
        }
        close(pfds[1]);
        wait(nullptr);
    }
    return 0;
}
