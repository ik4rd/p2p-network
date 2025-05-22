cmake_minimum_required(VERSION 3.30)
project(p2p)

set(CMAKE_CXX_STANDARD 20)
find_package(Boost REQUIRED COMPONENTS system)

include_directories(${Boost_INCLUDE_DIRS} include)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(p2p src/main.cpp
        src/message.cpp
        include/message.hpp
        src/peer.cpp
        include/peer.hpp
        src/network.cpp
        include/network.hpp
        include/thread_pool.hpp
)

target_link_libraries(p2p ${Boost_LIBRARIES})
