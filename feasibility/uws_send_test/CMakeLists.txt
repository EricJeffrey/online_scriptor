cmake_minimum_required(VERSION 3.0.0)
project(uWsSendTest VERSION 0.1.0)


add_definitions("-std=c++17")
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(uWsSendTest main.cpp)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
target_link_libraries(${PROJECT_NAME} pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

