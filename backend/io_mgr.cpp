#include "io_mgr.hpp"

event_base *IOMgr::base = nullptr;
int IOMgr::ioSock = -1;
TaskIOFdHelper IOMgr::taskIOFdHelper;

void IOMgr::start(int unixSock) {}

void IOMgr::addFds(array<int, 3> fds) {}

void IOMgr::removeFds(array<int, 3> fds) {}

void IOMgr::enableRedirect(array<int, 2> fds) {}

void IOMgr::disableRedirect(array<int, 2> fds) {}

void IOMgr::putToStdin(int fd, string buf) {}