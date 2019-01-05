cmake_minimum_required(VERSION 3.7)

project(wss-client)

#set (CMAKE_CXX_FLAGS "-O3")
set (CMAKE_CXX_FLAGS "-g")

include_directories(/mnt/store/current/src/boost_1_67_0 /mnt/store/current/src/boost_1_67_0/boost /usr/local/Cellar/boost/1.67.0_1/include /usr/local/Cellar/openssl/1.0.2o_2/include /usr/local/Cellar/libwebsockets/2.4.2_2/include)

set(CMAKE_CXX_STANDARD 14)

link_directories(/usr/local/lib /usr/local/Cellar/boost/1.67.0_1/lib /usr/local/Cellar/openssl/1.0.2o_2/lib /usr/local/Cellar/libwebsockets/2.4.2_2/lib)

add_executable(wss-client wss-client.cpp markets-loader.cpp markets-loader.h)

target_link_libraries(wss-client boost_system websockets stdc++ ssl crypto pthread)
