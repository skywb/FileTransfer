cmake_minimum_required(VERSION 3.10)

aux_source_directory(. DIR_LIB_SRCS)



#add_library(recv SHARED ${DIR_LIB_SRCS})
add_library(recv STATIC ${DIR_LIB_SRCS})
#target_link_libraries(server)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
