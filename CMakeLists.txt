cmake_minimum_required(VERSION 2.8)
project(ffdl)

#add_definitions(-DPROTO_BUF_SUPPORT)
add_definitions(-g)
add_definitions(-std=c++11)
add_definitions(-DENABLE_LOG_CONSOLE)


set(FFNET_ROOT_DIR ${PROJECT_SOURCE_DIR}/../ffnet/network)

include_directories(${PROJECT_SOURCE_DIR}/src ${FFNET_ROOT_DIR})
link_directories(${FFNET_ROOT_DIR}/lib)
message(${FFNET_ROOT_DIR}/lib)

set(cmn_src src/common/common.h
    src/common/types.h
    src/pkgs/pkgs.h
    src/pkgs/ack_nodes.cpp
    src/pkgs/cmd_send.cpp
    src/pkgs/file_send.cpp
    src/pkgs/heart_beat.cpp
    src/pkgs/req_nodes.cpp
    src/utils/utils.h
    src/utils/utils.cpp
  )

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_executable(dl_master ${cmn_src} src/tools/dl_master.cpp)
target_link_libraries(dl_master curl dl network boost_thread boost_date_time boost_system boost_serialization pthread)

add_executable(dl_worker ${cmn_src} src/tools/dl_worker.cpp)
target_link_libraries(dl_worker curl network boost_thread boost_date_time boost_system boost_serialization pthread)

add_executable(rm_master ${cmn_src} src/tools/rm_master.cpp)
target_link_libraries(rm_master curl network boost_thread boost_date_time boost_system boost_serialization pthread)

add_executable(slave ${cmn_src} src/tools/slave.cpp)
target_link_libraries(slave curl network boost_thread boost_date_time boost_system boost_serialization pthread)

add_executable(test_file_send ${cmn_src} src/tools/test_file_send.cpp)
target_link_libraries(test_file_send  curl network boost_thread boost_date_time boost_system boost_serialization pthread)
