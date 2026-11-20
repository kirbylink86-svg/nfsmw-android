cmake_minimum_required(VERSION 3.22.1)
project(nfsmw_native_diagnostics LANGUAGES C CXX)

if(NOT ANDROID)
  message(FATAL_ERROR "This diagnostic target must be configured with the Android NDK toolchain.")
endif()

if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
  message(FATAL_ERROR "Expected arm64-v8a, got: ${ANDROID_ABI}")
endif()

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)

set(SDL_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../sdk/thirdparty/sdl3")
if(NOT EXISTS "${SDL_SOURCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "SDL3 source is missing at ${SDL_SOURCE_DIR}")
endif()

add_subdirectory("${SDL_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/sdl3" EXCLUDE_FROM_ALL)

find_library(ANDROID_LOG_LIBRARY log REQUIRED)
find_library(ANDROID_VULKAN_LIBRARY vulkan REQUIRED)

add_library(nfsmw_native_diagnostics SHARED native_diagnostics.cpp)
target_compile_features(nfsmw_native_diagnostics PRIVATE cxx_std_17)
target_compile_options(nfsmw_native_diagnostics PRIVATE -Wall -Wextra -Werror)
target_link_libraries(nfsmw_native_diagnostics PRIVATE
  SDL3::SDL3-shared
  ${ANDROID_LOG_LIBRARY}
  ${ANDROID_VULKAN_LIBRARY}
)
