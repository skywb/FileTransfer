cmake_minimum_required(VERSION 3.10)


aux_source_directory(. DIR_SRC)

add_executable(LostPackageVec LostPackageVec_test.cpp)

target_link_libraries(LostPackageVec send util zip recv gtest gmock gtest_main pthread)

