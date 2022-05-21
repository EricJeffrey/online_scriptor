#include <iostream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

using namespace std;

int main(int argc, char const *argv[]) {
    cout << "Parent Started" << endl;
    string tmps = " :hello world: ";
    pid_t pid = fork();
    if (pid == -1) {
        cout << "Fork Failed" << endl;
        return 1;
    } else if (pid == 0) {
        cout << "Child Started" << endl;
        pid_t mypid = getpid();
        tmps = to_string(mypid) + tmps + to_string(pid);
        sleep(2);
        cout << tmps << endl;
        exit(233);
    } else {
        pid_t mypid = getpid();
        tmps = to_string(mypid) + tmps + to_string(pid);
        int childstatus = 0;
        if (wait(&childstatus) == -1) {
            cout << "Wait Failed" << endl;
            return 1;
        }
        cout << "Child Exited with code: " << WEXITSTATUS(childstatus) << endl;
        cout << tmps << endl;
        return 0;
    }
    return 0;
}
