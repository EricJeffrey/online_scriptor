
### 可行性探索

#### libevent尝试

监听标准输入与管道内容，另一个进程每隔一段时间向管道中写入数据 -- [libevent_test](libevent_test/main.c)

#### 子进程重定向STDIO到管道
 
主进程创建管道，将子进程的STDIO重定向到管道；然后使用libevent，监控子进程的STDIO，并将输出数据存储到队列，并向子进程的标准输入发送内容。 -- [stdio_redirect_test](stdio_redirect_test/main.c)

#### uwebsocket

主进程使用uwebsocket创建http接口，注册websocket接口。客户端建立连接后子线程每隔一段时间向ws发送数据。 -- [uws_send_test](uws_send_test/main.cpp)

#### Unix数据报套接字

主进程socketpair创建Unix流套接字对，fork，然后使用libevent监听读事件，并在读取到数据后sleep然后echo。
子进程使用套接字连接到主进程，使用libevent监听读事件，并响应，并创建子线程每隔一秒发送数据。 -- [unixsock_test](unixsock_test/main.cpp)

#### leveldb+json

主进程创建leveldb数据库，创建子进程，然后每隔2s向其中写入json格式化后的字符串，并读取上一次的内容看看是否被修改。
子进程每隔3s读取并修改一个key对应的内容。 -- [leveldb_test](/leveldb_test/main.cpp)

#### libevent+定时任务

使用libevent实现定时执行的任务 -- [libevent_timer](libevent_timer/main.cpp)

#### libevent+多线程+发送事件

主线程每隔2s向子线程发送一个事件，包含字符串
子线程使用libevent，设置EVLOOP_NO_EXIT_ON_EMPTY，等待主线程事件，并输出字符串值
使用libevent实现定时执行的任务 -- [event_multithread](event_multithread/main.cpp)

#### libevent+信号

使用libevent的信号事件确定子进程退出时是否会收到信号并触发回调

主进程创建libevent对象，然后添加信号事件，注册SIGCHILD的回调
子进程执行输出后退出。 -- [event_signal](event_signal/main.cpp)
