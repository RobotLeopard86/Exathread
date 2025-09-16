cmake_minimum_required(VERSION 3.10)
project(Exathread)

add_library(Exathread INTERFACE)
target_include_directories(Exathread INTERFACE include)
target_compile_features(Exathread INTERFACE cxx_std_20)

enable_testing()
add_executable(testeroo test/testeroo.cpp)
target_link_libraries(testeroo Exathread)
add_test(NAME Testeroo COMMAND testeroo WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test)