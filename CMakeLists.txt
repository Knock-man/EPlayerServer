cmake_minimum_required(VERSION 3.0) #指定cmake版本
project(EPlayer) #指定项目名称
set(CMAKE_CXX_STANDARD 11) #指定C++版本
set(CMAKE_BUILD_TYPE Debug)  



include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include) 
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/Socket) #指定头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/DB) 
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/http) 
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/encrypt)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/thread)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/Server)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/logger)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/process)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/public)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin) #指定生成可执行文件存储目录

#使用搜索方式2，指定文件类型

file(GLOB SRC_main ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*) 
file(GLOB SRC_sqlite3 ${CMAKE_CURRENT_SOURCE_DIR}/src/DB/*.c*)
file(GLOB SRC_http ${CMAKE_CURRENT_SOURCE_DIR}/src/http/*.c*)
file(GLOB SRC_socket ${CMAKE_CURRENT_SOURCE_DIR}/src/Socket/*.cpp)
file(GLOB SRC_encrypt ${CMAKE_CURRENT_SOURCE_DIR}/src/encrypt/*.cpp)
file(GLOB SRC_thread ${CMAKE_CURRENT_SOURCE_DIR}/src/thread/*.c*)
file(GLOB SRC_server ${CMAKE_CURRENT_SOURCE_DIR}/src/Server/*.c*)
file(GLOB SRC_logger ${CMAKE_CURRENT_SOURCE_DIR}/src/logger/*.c*)
file(GLOB SRC_process ${CMAKE_CURRENT_SOURCE_DIR}/src/process/*.c*)

add_executable(app ${SRC_process} ${SRC_main} ${SRC_sqlite3}  ${SRC_http} ${SRC_socket} ${SRC_encrypt} ${SRC_thread} ${SRC_logger} ${SRC_server}) #执行命令

target_link_libraries(app pthread mysqlclient crypto)