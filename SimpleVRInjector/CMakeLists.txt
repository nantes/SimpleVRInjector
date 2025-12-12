cmake_minimum_required(VERSION 3.20)
project(SimpleVRInjector VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Dependencies
include(FetchContent)

# MinHook (for hooking DX11)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

# OpenXR SDK (for VR)
FetchContent_Declare(
    openxr
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
    GIT_TAG        release-1.0.32
)
# OpenXR build options
set(BUILD_TESTS OFF CACHE INTERNAL "")
set(BUILD_LOADER ON CACHE INTERNAL "")
set(DYNAMIC_LOADER ON CACHE INTERNAL "")
FetchContent_MakeAvailable(openxr)

# Project Modules
add_subdirectory(src/Hook)
add_subdirectory(src/Launcher)
add_subdirectory(tests)

message(STATUS "Build setup complete for SimpleVRInjector")
