cmake_minimum_required(VERSION 3.30)
project(p2p_network)

set(CMAKE_CXX_STANDARD 20)
find_package(Boost REQUIRED COMPONENTS system)

include_directories(${Boost_INCLUDE_DIRS} include)

add_executable(p2p_network src/main.cpp
        src/message.cpp)

target_link_libraries(p2p_network ${Boost_LIBRARIES})
