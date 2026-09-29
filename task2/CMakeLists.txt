cmake_minimum_required(VERSION 3.10)
project(task2)


find_package(OpenCV REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(main_hpp
                src/main_hpp.cpp
                src/Pic.cpp 
                src/Video.cpp 
                src/Capture.cpp 
                )
include_directories(include)

target_include_directories(main_hpp PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(main_hpp PRIVATE ${OpenCV_LIBS})