cmake_minimum_required(VERSION 3.0.0)
project(main VERSION 0.1.0)

add_compile_options("-std=c++17" "-Wall")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(main main.cpp)

target_link_libraries(main ${CONAN_LIBS})
