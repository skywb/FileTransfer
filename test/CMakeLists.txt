cmake_minimum_required(VERSION 3.10)


aux_source_directory(. DIR_SRC)

add_executable(testFileSend testFileSend.cpp)
add_executable(testRecv testRecv.cpp)
add_executable(testFileSendControl testFileSendControl.cpp)
add_executable(testConnecter testConnecter.cpp)

target_link_libraries(testFileSend send recv util pthread)
target_link_libraries(testRecv send recv util pthread)
target_link_libraries(testFileSendControl send recv util pthread)
target_link_libraries(testConnecter send recv util pthread)

