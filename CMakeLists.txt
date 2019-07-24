cmake_minimum_required(VERSION 3.10)


project(project)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -D DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

include_directories(".")
SET(PROJECT_ROOT_PATH ".")

aux_source_directory(. DIR_SRC)

add_subdirectory(send)
add_subdirectory(recv)
add_subdirectory(util)
add_subdirectory(test)


#SET(LIBRARY_OUTPUT_PATH lib)


#add_executable(server server.cpp)
#add_executable(main ${DIR_SRC})

#target_link_libraries(server IM Util Reactor dao mysqlcppconn pthread)


#enable_testing()
#
#add_test(NAME TDao COMMAND testDao)
