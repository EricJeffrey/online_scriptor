#if !defined(TASK_MGR)
#define TASK_MGR

#include "event2/bufferevent.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"

// 任务管理模块不同线程之间的共享对象
struct TaskMgr {
    event_base *cmdMgrBase;
    event_base *scriptMgrBase;
    int unixSock;

    TaskMgr(event_base *cmbase, event_base *smbase, int us) : cmdMgrBase(cmbase),
                                                              scriptMgrBase(smbase), unixSock(us) {}
};

#endif // TASK_MGR
