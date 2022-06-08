## 任务管理模块

**注** 以下内容已经过时，仅供参考

### 外部通信套接字

1. CommandSock - 同步，传递用户的 添加/启动/停止/删除/获取详细信息/重定向IO/取消重定向IO/添加到任务输入 命令到任务管理模块
2. IOSock - 异步，传递脚本的输出内容

### 子模块

1. 命令处理 - 处理任务相关的命令 以及 脚本进程退出的事件
2. IO重定向 - 处理任务输出的重定向数据

#### 命令处理流程 - CmdMgr

1. `startCmdMgr(cmdSock)`启动命令处理
   1. 初始化libevent
   2. 添加SIGCHILD事件及处理函数
   3. 添加cmdSock读事件及处理函数
2. SIGCHILD处理
   1. 使用`wait`回收并获取`pid, exitCode`
   2. `runningTaskHelper.getFdsByPid(pid)`获取文件描述符
   3. `iomgr.removeFd(stdin, stdout, stderr)`通知移除fds
   4. `runningTaskHelper.removeByPid(pid)`移除进程fd
   5. `dbHelper.updateTask()`更新退出码
   6. 返回
3. cmdSock事件
   1. `bufferHelper.tryFillCmdMsg()`解析为CmdMsg对象
   2. 根据`cmdType`分发处理函数
   3. 返回分发后的处理结果
4. cmd添加任务
   1. `dbHelper.addTask(type, code)`
   2. 返回
5. cmd启动任务
   1. `dbHelper.getTaskInfo(taskId)`获取任务信息
   2. `scripthelper.startTask(taskId, type, script)`启动脚本，得到`ok, pid, iofds[3]`
   3. `iomgr.addFd(stdin, stdout, stderr)`添加脚本的描述符到IO重定向中
   4. `runningTaskHelper.add(taskId, pid, fds)`保存信息
   5. `dbHelper.updateTask()`
   6. 返回
6. cmd停止任务
   1. `dbHelper.getTaskInfo(taskId)`获取任务pid
   2. `kill(SIGKILL, pid)`
   3. 返回
7. cmd删除任务
   1. `runningTaskHelper.isTaskRunning(taskId)`确定任务是否运行
   2. 如果在运行 - cmd停止任务
   3. `dbHelper.removeTask(taskId)`
   4. 返回
8. cmd重定向IO
   1. `runningTaskHelper.isTaskRunning(taskId)`确定任务是否运行
   2. `runningTaskHelper.getFdsByTask(taskId)`获取fd
   3. `iomgr.startRedirect(fds[3])`通知重定向数据
9. cmd停止重定向
   1. `runningTaskHelper.isTaskRunning(taskId)`确定任务是否运行
   2. `runningTaskHelper.getFdsByTask(taskId)`获取fd
   3. `iomgr.stopRedirect(fds[3])`通知重定向数据
10. cmd添加到任务输入
    1.  `runningTaskHelper.getFdsByTask(taskId)`获取标准输入fd
    2.  `iomgr.addToStdin(fd, content)`添加到输入

#### IO重定向流程 - IOMgr

1. `startIOMgr(ioSock)`
   1. 初始化libevent
   2. 添加外部接口的处理函数
   3. 添加脚本输出fd的处理函数
2. 外部接口 - addFd
   1. `bufferevent_new, setcb, enable`
   2. `taskFdHelper.addFd(fd)`保存
   3. `taskFdHelper.setBev(bev)`
3. 外部接口 - removeFd
   1. `bufferevent_free(taskFdHelper.getBev(fd))`
   2. `taskFdHelper.removeFd(fd)`移除
4. 外部接口 - startRedirect
   1. `taskFdHelper.setDoRedirect(fd, true)`
5. 外部接口 - stopRedirect
   1. `taskFdHelper.setDoRedirect(fd, false)`
6. 外部接口 - addToStdin
   1. `write(stdinFd, buf, buf.size)`写入到用户的输入数据，若会阻塞则丢弃
7. 脚本fd的输出
   1. 封装为`IOMsg`格式的JSON数据并字符串化`msgJson`
   2. `taskFdHelper.getDoRedirect(fd)`判断是否转发
   3. 若是，`taskFdHelper.getBev(fd)`，然后`bufferevent_write(ioSoc, msgJson.data, msgJson.size)`

#### 数据格式与接口

```

CommandMsg
    cmdType;
    taskId;
    title;
    scriptCode;
    scriptType;
    interval;
    maxTimes;
    stdinContent;

CommandRes
   bool ok
   int taskId
   pid_t pid
   Task task

IOEventMsg
   enum cmd
   int fd
   bytes buf

CmdMgr {
   start(cmdSock)
   onSIGCHILD()
   onCmdSockRead()
   onCmdSockEvent()
   addTask()
   startTask()
   stopTask()
   deleteTask()
   enableIORedirect(taskId)
   disableIORedirect(taskId)
   addToTaskSTDIN()
}

IOMgr {
   start(ioSock)
   addFds(taskId, fds)
   removeFds(taskId, fds)
   enableRedirect(fds)
   disableRedirect(fds)
   addToStdin(fd, buf, size)
   onFdOutput()
}

bufferHelper {
   tryFillCmdMsg(buf, n)
   tryFillIOMsg(buf, n)
}

runningTaskHelper {
   add(taskId, pid, fds)
   isTaskRunning(taskId)
   getFdsByPid(pid)
   getFdsByTask(taskId)
   removeByPid(pid)
}

dbHelper {
   addTask(task)
   updateTask(taskId, newTask)
   deleteTask(taskId)
   getTask(taskId)
}

scriptHelper {
   startTask(taskId, type, script)
}

taskIOFdHelper {
   add(taskId, fd)
   remove(fd)
   enableRedirect(fd)
   disableRedirect(fd)
   isRedirectEnabled(fd)
   getBufferEv(fd)
   getTaskId(fd)
}

```

### 注意事项

1. stdin的写端应当标记为`NONBLOCK`，避免`write`阻塞
2. 向stdin写入的数据应当保证不会阻塞，否则会直接丢弃
