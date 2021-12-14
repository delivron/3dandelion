cmake_minimum_required(VERSION 3.2)

project(3Dandelion LANGUAGES CXX)

set(EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/external)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(external)
add_subdirectory(source)
