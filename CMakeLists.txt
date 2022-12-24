cmake_minimum_required(VERSION 3.16)
project(scratch)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0115 NEW)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall")

set(scratch_NAME Scratch)
set(scratch_VERSION_MAJOR 0)
set(scratch_VERSION_MINOR 1)

set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
set(scratch_DIR ${CMAKE_INSTALL_PREFIX})
set(scratch_DATADIR ${CMAKE_INSTALL_PREFIX}/share)

include_directories("${PROJECT_SOURCE_DIR}")

# configure_file(
#         "${PROJECT_SOURCE_DIR}/config.h.in"
#         "${PROJECT_SOURCE_DIR}/config.h"
# )

find_package(Readline)
if(READLINE_FOUND)
    set(LIBS ${READLINE_LIBRARIES} ${LIBS})
    set(INCLUDES ${INCLUDES} ${READLINE_INCLUDE_DIRS})
endif(READLINE_FOUND)

add_executable(
        scratch
        App.cpp
        Document.cpp
        Editor.cpp
        Main.cpp
        Menu.cpp
        MessageBar.cpp
        StatusBar.cpp
        Widget.cpp
        WidgetStack.cpp WidgetStack.h LogViewer.cpp LogViewer.h)

target_link_libraries(
        scratch
        ncurses
)

target_compile_features(scratch PUBLIC cxx_std_20)

install(TARGETS scratch
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)
