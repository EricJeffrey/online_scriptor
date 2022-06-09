
文件目录
- `./*.hpp ./*.cpp` 后端CPP代码
- `conanfile.txt, build/*` cmake和conan的文件，用来管理依赖构建项目

编译

- `mkdir build`
- `cd build && conan install .. && cd ..`
- `cmake -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc-9 -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++-9 -DCMAKE_BUILD_TYPE:STRING=Debug -S./ -B build/`，默认Debug模式，可以使用`cmake -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=/bin/gcc-9 -DCMAKE_CXX_COMPILER:FILEPATH=/bin/g++-9 -S./ -B./build"`修改为Release模式
- `cmake --build ./build -j 40`

运行

- `./build/bin/main`

修改端口和文件路径

- 直接修改`config.hpp`中的内容重新编译即可
