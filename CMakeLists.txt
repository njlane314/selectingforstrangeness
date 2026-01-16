cmake_minimum_required(VERSION 3.16)
project(MyAnalysis LANGUAGES CXX)

find_package(ROOT REQUIRED COMPONENTS Core RIO Tree Hist Graf Gpad ROOTDataFrame ROOTVecOps)

set(CMAKE_CXX_STANDARD ${ROOT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(ana SHARED)

target_include_directories(ana PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(ana PUBLIC
  ROOT::Core
  ROOT::RIO
  ROOT::Tree
  ROOT::Hist
  ROOT::Graf
  ROOT::Gpad
  ROOT::ROOTDataFrame
  ROOT::ROOTVecOps
)
