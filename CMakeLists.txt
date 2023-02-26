cmake_minimum_required(VERSION 3.16)
project(scratch)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0115 NEW)
cmake_policy(SET CMP0135 NEW)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall")

set(scratch_NAME Scratch)
set(scratch_VERSION_MAJOR 0)
set(scratch_VERSION_MINOR 1)

set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
set(scratch_DIR ${CMAKE_INSTALL_PREFIX})
set(scratch_DATADIR ${CMAKE_INSTALL_PREFIX}/share)

set(INCLUDES "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/include")

find_package(SDL2 REQUIRED)
find_package(SDL2_gfx REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

set(INCLUDES ${INCLUDES} "${SDL2_INCLUDE_DIRS}" "${SDL2_TTF_INCLUDE_DIRS}")

if(FETCH_OBELIXLIBS)
    include(FetchContent)
    FetchContent_Declare(
            obelixlibs
            GIT_REPOSITORY git@github.com:JanDeVisser/obelixlibs.git
            GIT_TAG main
    )
    FetchContent_MakeAvailable(obelixlibs)
else()
    set(INCLUDES ${INCLUDES} ${PROJECT_SOURCE_DIR}/obelixlibs)
    add_subdirectory(${PROJECT_SOURCE_DIR}/obelixlibs)
endif()

include_directories(
        "${INCLUDES}"
        "${SDL2_INCLUDE_DIRS}"
        "${SDL2_TTF_INCLUDE_DIRS}"
)

add_executable(
        scratch
        App.cpp
        Document.cpp
        EditAction.cpp
        Editor.cpp
        EditorState.cpp
        Geometry.h
        Gutter.cpp
        Key.cpp
        Scratch.cpp
        SDLContext.cpp
        StatusBar.cpp
        Text.cpp
        Commands/ArgumentHandler.cpp
        Commands/Command.cpp
        Commands/CommandHandler.cpp
        Commands/DefaultArgumentHandler.cpp
        Parser/CPlusPlus.cpp
        Parser/PlainText.cpp
        Scribble/Interp/Value.cpp
        Scribble/Parser.cpp
        Scribble/Processor.cpp
        Scribble/Scribble.cpp
        Scribble/Syntax/ControlFlow.cpp
        Scribble/Syntax/Expression.cpp
        Scribble/Syntax/Function.cpp
        Scribble/Syntax/literal.cpp
        Scribble/Syntax/Statement.cpp
        Scribble/Syntax/Syntax.cpp
        Scribble/Syntax/Variable.cpp
        Widget/Frame.cpp
        Widget/Layout.cpp
        Widget/ModalWidget.cpp
        Widget/Widget.cpp
        Widget/WindowedWidget.cpp
        Widget/WidgetContainer.cpp
        Scribble/Interp/Operator.h Scribble/Interp/ExpressionResult.cpp Scribble/Interp/ExpressionResult.h Scribble/Interp/Interpreter.cpp Scribble/Interp/Interpreter.h)

target_link_libraries(
        scratch
        oblcore
        obllexer
        SDL2::Main
        SDL2::GFX
        SDL2::Image
        SDL2::TTF
)

target_compile_features(scratch PUBLIC cxx_std_20)

install(TARGETS scratch
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)
