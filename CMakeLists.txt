cmake_minimum_required(VERSION 3.16)
project(scratch)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0115 NEW)
cmake_policy(SET CMP0135 NEW)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall")

set(scratch_NAME Scratch)
set(scratch_VERSION_MAJOR 0)
set(scratch_VERSION_MINOR 1)

set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
set(scratch_DIR ${CMAKE_INSTALL_PREFIX})
set(scratch_DATADIR ${CMAKE_INSTALL_PREFIX}/share)

include(FetchContent)
FetchContent_Declare(
        obelixlibs
        GIT_REPOSITORY git@github.com:JanDeVisser/obelixlibs.git
        GIT_TAG main
)
FetchContent_MakeAvailable(obelixlibs)
include_directories("${PROJECT_SOURCE_DIR}" "${obelixlibs_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/include")

find_package(Readline)
if(READLINE_FOUND)
    set(LIBS ${READLINE_LIBRARIES} ${LIBS})
    set(INCLUDES ${INCLUDES} ${READLINE_INCLUDE_DIRS})
endif(READLINE_FOUND)

add_executable(
        scratch
        App.cpp
        Display.cpp
        Document.cpp
        Editor.cpp
        Main.cpp
        Menu.cpp
        MessageBar.cpp
        StatusBar.cpp
        Widget.cpp
)

target_link_libraries(
        scratch
        oblcore
        obllexer
)

target_compile_features(scratch PUBLIC cxx_std_20)

add_executable(escape escape.c)

install(TARGETS scratch escape
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)
