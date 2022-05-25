
websocket测试，conan + CMake + uwebsocket

- 配置：`mkdir build` -> `cd build` -> `conan install ..` -> `cmake -DCMAKE_CXX_COMPILER=g++-9 ..` 如果出现
  
  ```
  -- Configuring done
  -- Generating done
  -- Build files have been written to: /home/....
  ```
  表示配置完成


- 编译：`cd build/` -> `cmake --build .`

- 运行：`./build/bin/uWsSendTest`