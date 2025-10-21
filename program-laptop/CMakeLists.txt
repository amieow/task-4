cmake_minimum_required(VERSION 3.10)
project(BridgeApp)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    serial/include
)

# file(GLOB SOURCES "src/*.cpp")
set(SOURCES
    src/main.cpp
)

set(LIBRARIES
    "D:/CMAKE_JSON/task-4-cmake/serial/visual_studio/x64/Release/serial.lib"
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
