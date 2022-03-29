project(VNE_CBS_Sim)

cmake_minimum_required (VERSION 2.6)
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -Wall -std=c++0x -pthread -m64")

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -Wall --std=c++0x ")

include_directories("../")
file(GLOB SOURCES "../*.cpp" "*.cpp")

# Find Boost
find_package(Boost)
include_directories( ${Boost_INCLUDE_DIRS} )

add_executable(VNE_CBS_Sim ${SOURCES})

