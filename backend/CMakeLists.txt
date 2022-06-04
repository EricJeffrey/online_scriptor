cmake_minimum_required(VERSION 3.0.0)
project(main VERSION 0.1.0)

add_compile_options("-std=c++17" "-Wall")
add_compile_definitions("TESTING")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

aux_source_directory(./ SOURCE_FILES)
add_executable(main ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
target_link_libraries(${PROJECT_NAME} pthread)
