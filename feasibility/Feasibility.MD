
### 可行性探索

相关代码单独放到新的子目录中，目录名要能够反映内容，例如与fork相关的可以放到``feasibilitiy/fork_test`。

#### 子进程重定向STDIO到管道

1. fork与pipe
2. 使用dup2重定向STDIO到pipe

#### libevent监听多个IO

1. libevent上手尝试
2. 监听多个文件描述符，添加/删除
3. 监听特定事件（读/写/关闭）

#### C/C++包管理器

1. Make/CMake基础使用
2. conan，vcpkg...

#### HTTP服务端

1. C/C++的HTTP服务器，支持websocket
2. GET/POST处理

#### JSON处理

1. C/C++的JSON处理库（rapidjson，nlohmann/json ...）

#### websocket连接

1. C/C++的websocket库（服务端）
2. 使用websocket与客户端通信，发送/接收消息，处理断开连接

#### MySQL数据库

1. C/C++连接MySQL数据库
2. 操作MySQL