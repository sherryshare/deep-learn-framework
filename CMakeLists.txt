cmake_minimum_required(VERSION 2.8)
project(ffdl)

#add_definitions(-DPROTO_BUF_SUPPORT)
add_definitions(-g)
#add_definitions(-std=c++11)
add_definitions(-DENABLE_LOG_CONSOLE)


set(FFNET_ROOT_DIR ${PROJECT_SOURCE_DIR}/../ffnet/network)

include_directories(${PROJECT_SOURCE_DIR}/src ${FFNET_ROOT_DIR})
link_directories(${FFNET_ROOT_DIR}/lib)
message(${FFNET_ROOT_DIR}/lib)

set(cmn_src src/pkgs/ack_nodes.cpp
    src/pkgs/cmd_send.cpp
    src/pkgs/heart_beat.cpp
    src/pkgs/req_nodes.cpp
    src/pkgs/file_send.cpp
    src/pkgs/pkgs.cpp
    src/utils/utils.cpp
  )
  
set(sae_src src/dsource/read.cpp
  src/dsource/divide.cpp
  src/nn/fbnn.cpp
  src/sae/sae.cpp
  src/sae/sae_from_config.cpp
  src/utils/math.cpp
  src/utils/matlib.cpp
  src/nn/arch.cpp)
  
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
#matio blaze 
set(cmn_libs network  boost_thread boost_system matio blaze curl)
ADD_LIBRARY(ffdl SHARED ${cmn_src} ${sae_src} )
#target_link_libraries(ffdl boost_system boost_date_time boost_thread)

#add_executable(t_divide src/tools/test_divide.cpp)
#target_link_libraries(t_divide curl ffdl matio blaze)

add_executable(dl_master src/tools/dl_master.cpp)
target_link_libraries(dl_master ffdl ${cmn_libs})

add_executable(dl_worker src/tools/dl_worker.cpp)
target_link_libraries(dl_worker ffdl ${cmn_libs})

add_executable(rm_master src/tools/rm_master.cpp)
target_link_libraries(rm_master ffdl ${cmn_libs})

add_executable(slave src/tools/slave.cpp)
target_link_libraries(slave ffdl ${cmn_libs})

#add_executable(test_file_send src/tools/test_file_send.cpp)
#target_link_libraries(test_file_send curl ffdl network boost_thread boost_date_time boost_system boost_serialization pthread)
