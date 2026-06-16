cmake_minimum_required(VERSION 3.15)
project(fate C)

add_compile_options(-g -Wall -Wextra -O2)

add_definitions(-DFATE_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/fate")
add_definitions(-DFATE_VERSION="1.0.0")

add_executable(fate src/fate.c)

include(GNUInstallDirs)

install(TARGETS fate DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY predictions/ DESTINATION ${CMAKE_INSTALL_DATADIR}/fate)
