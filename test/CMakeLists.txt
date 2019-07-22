cmake_minimum_required(VERSION 3.10)


aux_source_directory(. DIR_SRC)

add_executable(testFileSend testFileSend.cpp)
add_executable(testReactor testReactor.cpp)
add_executable(testConnect testConnect.cpp)
add_executable(testRecv testRecv.cpp)

target_link_libraries(testFileSend send reactor pthread)
target_link_libraries(testReactor reactor send pthread)
target_link_libraries(testConnect reactor send pthread)
target_link_libraries(testRecv reactor send recv pthread)

