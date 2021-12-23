cmake_minimum_required(VERSION 3.2)

project(3Dandelion LANGUAGES CXX)

set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/external)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT 3Dandelion)

add_subdirectory(external)
add_subdirectory(source)
