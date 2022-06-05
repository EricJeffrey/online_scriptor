#include "http_mgr.hpp"
#include "task_mgr.hpp"
#include "test_task_mgr.hpp"

#include "fmt/format.h"

int main(int argc, char const *argv[]) {
    fmt::print("Hello World\n");
    HttpMgr::start(-1, -1);
    return 0;
}
