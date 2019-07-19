cmake_minimum_required(VERSION 3.10)

aux_source_directory(. DIR_LIB_SRCS)



add_library(reactor SHARED ${DIR_LIB_SRCS})

SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
