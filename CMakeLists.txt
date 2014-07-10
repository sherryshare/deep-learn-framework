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

set(cmn_src src/pkgs/ack_nodes.cpp
    src/pkgs/cmd_send.cpp
    src/pkgs/file_send.cpp
    src/pkgs/heart_beat.cpp
    src/pkgs/req_nodes.cpp
    src/pkgs/sae_handles.cpp
  )
  
set(sae_src src/dsource/read.cpp
  src/dsource/divide.cpp
  src/nn/fbnn.cpp
  src/sae/sae.cpp
  src/sae/sae_from_config.cpp
  src/utils/math.cpp
  src/utils/matlib.cpp
  src/utils/utils.cpp
  src/nn/arch.cpp)
  
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

ADD_LIBRARY(ffdl SHARED ${sae_src} ${cmn_src} )
ADD_LIBRARY(ffdl_s STATIC ${sae_src} ${cmn_src} )
target_link_libraries(ffdl matio blaze boost_system boost_date_time boost_thread)
target_link_libraries(ffdl_s matio blaze boost_system boost_date_time boost_thread)
SET_TARGET_PROPERTIES(ffdl_s PROPERTIES OUTPUT_NAME "ffdl")

add_executable(t_divide src/tools/test_divide.cpp)
target_link_libraries(t_divide curl dl ffdl matio blaze)

add_executable(dl_master src/tools/dl_master.cpp)
target_link_libraries(dl_master curl dl ffdl matio blaze network boost_thread boost_date_time boost_system boost_serialization pthread)

add_executable(dl_worker src/tools/dl_worker.cpp)
target_link_libraries(dl_worker curl dl ffdl network boost_thread boost_date_time boost_system boost_serialization pthread)

add_executable(rm_master src/tools/rm_master.cpp)
target_link_libraries(rm_master curl dl ffdl network boost_thread boost_date_time boost_system boost_serialization pthread)

add_executable(slave src/tools/slave.cpp)
target_link_libraries(slave curl dl ffdl network boost_thread boost_date_time boost_system boost_serialization pthread)

add_executable(test_file_send src/tools/test_file_send.cpp)
target_link_libraries(test_file_send curl dl ffdl network boost_thread boost_date_time boost_system boost_serialization pthread)
