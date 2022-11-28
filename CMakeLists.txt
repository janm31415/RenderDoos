cmake_minimum_required(VERSION 3.10)

set(CMAKE_OSX_DEPLOYMENT_TARGET "14.00")

project (RenderDoos)

set(RENDERDOOS_PLATFORM "win32" CACHE STRING "Choose your target platform: win32, linux, macos or ios.")
set(RENDERDOOS_ARCHITECTURE "x64" CACHE STRING  "Choose your target architecture: x64 compatible pc or arm.")

set_property(CACHE RENDERDOOS_PLATFORM PROPERTY STRINGS win32 linux macos ios)
set_property(CACHE RENDERDOOS_ARCHITECTURE PROPERTY STRINGS x64 arm)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

if (${RENDERDOOS_PLATFORM} STREQUAL "ios")
set(CMAKE_OSX_SYSROOT iphoneos)
endif (${RENDERDOOS_PLATFORM} STREQUAL "ios")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(RenderDoos)

if (${RENDERDOOS_PLATFORM} STREQUAL "win32")
add_subdirectory(glew)
set_target_properties (RenderDoosGlew PROPERTIES FOLDER glew)
endif (${RENDERDOOS_PLATFORM} STREQUAL "win32")
if (${RENDERDOOS_PLATFORM} STREQUAL "linux")
add_subdirectory(glew)
set_target_properties (RenderDoosGlew PROPERTIES FOLDER glew)
endif (${RENDERDOOS_PLATFORM} STREQUAL "linux")


