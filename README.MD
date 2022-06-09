
# 在线脚本执行器

## 功能

- 创建/启动/停止/删除脚本，支持Python和Bash脚本
- 支持设置脚本执行的间隔时间和最多执行次数
- 能够查看脚本的实时输出信息
- 能够将数据发送到脚本的标准输入

## 部署运行

**注**：受限于进程结构，目前仅能保证 *单用户* 操作的正确性

1. 编译后端 - 参考[backend/README.MD](backend/README.MD)
2. 复制前端代码`frontend/`下的内容到 *静态文件目录* 的`static`文件夹，默认为`/data/online_scriptor/static/`
3. 根据[config.hpp](config.hpp)的内容
  - 创建静态文件目录与数据库目录，默认目录：
    - `mkdir /data/online_scriptor/`
    - `mkdir /data/online_scriptor/static/`
    - `mkdir /data/online_scriptor/task_db/`
  - 确保Python解释器目录与Bash目录正确
4. 运行后端可执行程序`backend/build/bin/main`
