cmake_minimum_required(VERSION 3.10)
project(Exathread)

add_library(Exathread INTERFACE)
target_include_directories(Exathread INTERFACE include)
target_compile_features(Exathread INTERFACE cxx_std_20)
set_target_properties(Exathread PROPERTIES PUBLIC_HEADER include/exathread.hpp)

enable_testing()
add_executable(testeroo test/testeroo.cpp)
target_link_libraries(testeroo Exathread)
add_test(NAME Testeroo COMMAND testeroo WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test)

include(GNUInstallDirs)
install(TARGETS Exathread PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/exathread)
