cmake_minimum_required(VERSION 3.5)
project(PS_9_MERRY)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    merry_pcd/include/pcl_utils/pcl_utils.h
    merry_pcd/src/find_indices_of_plane_from_patch.cpp
    merry_pcd/src/find_plane_coke_can_merry.cpp
    merry_pcd/src/pcd_snapshot.cpp
    merry_pcd/src/pcl_utils.cpp)

add_executable(PS_9_MERRY ${SOURCE_FILES})