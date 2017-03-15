cmake_minimum_required(VERSION 3.0)
project(example-server)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")

find_package(Threads REQUIRED)
find_package(LibWebRTC REQUIRED)
include(${LIBWEBRTC_USE_FILE})

include_directories("src/")
include_directories("lib/")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB PROJECT_CONFIGS CMakeLists.txt
                         .gitattributes
                         .gitignore
                         .gitmodules)

add_executable(server_exe src/main.cpp src/observers.h)
set_target_properties(server_exe PROPERTIES OUTPUT_NAME example-server
    RUNTIME_OUTPUT_DIRECTORY /${CMAKE_BINARY_DIR}/bin/)
target_link_libraries(server_exe ${LIBWEBRTC_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
    boost_system pthread)

