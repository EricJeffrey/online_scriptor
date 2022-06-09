
# 后端结构与编译

## 文件结构

- `./*.hpp ./*.cpp` 后端CPP代码
  - `config.hpp` 包含了相关的配置代码，编译前先查看/修改该文件内容
- `conanfile.txt, CMakeLists.txt, build/*` cmake和conan的文件，用来管理依赖构建项目

## 依赖安装

**注**：仅在`Ubuntu20.04`环境下的`g++9.4.0`编译过，其它Linux发行版暂未测试

- cmake `apt install cmake`
- python3与pip3 `apt install python3-pip`
- conan - `pip3 install conan`
  - 安装完成后执行`conan profile update settings.compiler.libcxx=libstdc++11 default`设置libcxx的ABI版本

## 编译

**注**：先查看并修改[config.hpp](config.hpp)以确保端口与目录可用

以下操作均在当前目录`backend`下执行

- 创建build目录 `mkdir build`
- 使用conan安装依赖 `cd build && conan install .. && cd ..`
- Debug模式`cmake -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc-9 -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++-9 -DCMAKE_BUILD_TYPE:STRING=Debug -S./ -B build/`
  - 也可以使用`cmake -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=/bin/gcc-9 -DCMAKE_CXX_COMPILER:FILEPATH=/bin/g++-9 -S./ -B./build"`修改为Release模式
- `cmake --build ./build -j 40`

## 后端进程/线程结构设计

![proc_thr_structure](misc/proc_thr_structure.png)
