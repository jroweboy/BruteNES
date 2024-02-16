
cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME brutenes)

project(${PROJECT_NAME} CXX)

# 1. Try system SDL3 package first
find_package(SDL3 QUIET)
if(SDL3_FOUND)
    message(STATUS "Using SDL3 via find_package")
endif()

# 2. Try using a vendored SDL library
if(NOT SDL3_FOUND AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/SDL/CMakeLists.txt")
    add_subdirectory(SDL)
    message(STATUS "Using SDL3 via add_subdirectory")
    set(SDL3_FOUND TRUE)
endif()

# 3. Download SDL, and use that.
if(NOT SDL3_FOUND)
    include(FetchContent)
    set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
    set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
    FetchContent_Declare(
        SDL
        SYSTEM
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main  # Replace this with a particular git tag or git hash
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    message(STATUS "Using SDL3 via FetchContent")
    FetchContent_MakeAvailable(SDL)
    set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
endif()

FetchContent_Declare(
    argparse
    SYSTEM
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable()

FetchContent_Declare(
    range-v3
    SYSTEM
    GIT_REPOSITORY "https://github.com/ericniebler/range-v3"
    GIT_TAG "a81477931a8aa2ad025c6bda0609f38e09e4d7ec"
)

FetchContent_MakeAvailable(
    argparse
    range-v3
)


add_executable(${PROJECT_NAME}
    inc/brutenes.h
    inc/bus.h
    inc/common.h
    inc/cpu.h
    inc/ines.h
    inc/scheduler.h
    inc/virtmem.h

    src/brutenes.cpp
    src/bus.cpp
    src/cpu.cpp
    src/ines.cpp
    src/main.cpp
    src/scheduler.cpp
    src/virtmem.cpp
)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3-static
    argparse
    range-v3
)

target_include_directories(${PROJECT_NAME} PUBLIC inc/)