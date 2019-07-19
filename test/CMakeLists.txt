cmake_minimum_required(VERSION 3.10)


aux_source_directory(. DIR_SRC)

add_executable(testFileSend testFileSend.cpp)
add_executable(testReactor testReactor.cpp)
add_executable(testConnect testConnect.cpp)

target_link_libraries(testFileSend send)
target_link_libraries(testReactor reactor send)
target_link_libraries(testConnect reactor send)

