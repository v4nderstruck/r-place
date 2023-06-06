cmake_minimum_required(VERSION 3.10)
project(rplace)

set(CMAKE_CXX_STANDARD 17)

include(ExternalProject)

set(INSTALL_DIR ${CMAKE_BINARY_DIR}/INSTALL)
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third-party)

set(Boost_LIBRARY_DIR ${INSTALL_DIR}/lib/boost/)
set(Boost_INCLUDE_DIR ${INSTALL_DIR}/include/)

ExternalProject_Add(
  boost
  URL ${THIRD_PARTY_DIR}/boost_1_82_0.tar.gz
  SOURCE_DIR ${CMAKE_BINARY_DIR}/boost
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ./bootstrap.sh  --prefix=${INSTALL_DIR}
  BUILD_COMMAND ./b2 install -j8 --without-mpi
  INSTALL_DIR ${INSTALL_DIR}
)
ExternalProject_Add(
  plog 
  URL ${THIRD_PARTY_DIR}/plog-1.1.9.tar.gz
  SOURCE_DIR ${CMAKE_BINARY_DIR}/plog
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND cp -r ${CMAKE_BINARY_DIR}/plog/include/plog ${INSTALL_DIR}/include/
  INSTALL_DIR ${INSTALL_DIR}
)
configure_file(${THIRD_PARTY_DIR}/CLI11.hpp ${INSTALL_DIR}/include/CLI11.hpp COPYONLY)
include_directories(${INSTALL_DIR}/include)

file(GLOB SRC
     "src/*.h"
     "src/*.cpp"
)

add_executable(rplace ${SRC})


