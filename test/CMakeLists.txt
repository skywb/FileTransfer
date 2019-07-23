cmake_minimum_required(VERSION 3.10)


aux_source_directory(. DIR_SRC)

add_executable(testFileSend testFileSend.cpp)
add_executable(testRecv testRecv.cpp)

target_link_libraries(testFileSend send pthread)
target_link_libraries(testRecv send recv pthread)

