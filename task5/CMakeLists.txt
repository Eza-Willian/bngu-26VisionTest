cmake_minimum_required(VERSION 3.10)
project(task5)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
add_executable(main src/main.cpp)
include_directories(include)

find_package(OpenCV REQUIRED)

target_link_libraries(main PRIVATE ${OpenCV_LIBS})
target_include_directories(main PRIVATE ${OpenCV_INCLUDE_DIRS})