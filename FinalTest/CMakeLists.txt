cmake_minimum_required(VERSION 3.10)
project(final_task)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

find_package(OpenCV REQUIRED)

add_executable(main 
                src/main.cpp
                motor/angle_uart.c
                motor/control.cpp)

target_link_libraries(main PRIVATE ${OpenCV_LIBS})
target_include_directories(main PRIVATE 
                            ${OpenCV_INCLUDE_DIRS}
                            ${PROJECT_SOURCE_DIR}/include
                            ${PROJECT_SOURCE_DIR}/motor)