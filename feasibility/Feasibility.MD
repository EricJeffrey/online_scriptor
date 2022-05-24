
### 可行性探索

#### libevent尝试

监听标准输入与管道内容，另一个进程每隔一段时间向管道中写入数据 -- [libevent_test](libevent_test/main.c)

#### 子进程重定向STDIO到管道
 
主进程创建管道，将子进程的STDIO重定向到管道；然后使用libevent，监控子进程的STDIO，并将输出数据存储到队列，并向子进程的标准输入发送内容。 -- [stdio_redirect_test](stdio_redirect_test/main.c)
