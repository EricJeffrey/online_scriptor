
文件目录
- `./*.hpp ./*.cpp` 后端CPP代码
- `conanfile.txt, build/*` cmake和conan的文件，用来管理依赖构建项目

编译

- `mkdir build`
- `cd build && conan install .. && cd ..`
- `cmake -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc-9 -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++-9 -B build/`
- `cmake --build ./build`

运行

- `./build/bin/main`

