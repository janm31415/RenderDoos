option(RENDERDOOS_SIMD "Turn off if you don't want to use SIMD instructions" ON)

set(HDRS
float.h
material.h
render_context.h
render_engine.h
types.h
    )

set(SRCS
float.cpp
material.cpp
render_context.cpp
render_engine.cpp
)

set(SHADERS
)

if (${RENDERDOOS_PLATFORM} STREQUAL "ios")
list(APPEND HDRS render_context_metal.h)
list(APPEND SRCS render_context_metal.cpp)
list(APPEND SHADERS shaders.metal)
endif (${RENDERDOOS_PLATFORM} STREQUAL "ios")
if (${RENDERDOOS_PLATFORM} STREQUAL "macos")
list(APPEND HDRS render_context_metal.h)
list(APPEND SRCS render_context_metal.cpp)
list(APPEND SHADERS shaders.metal)
endif (${RENDERDOOS_PLATFORM} STREQUAL "macos")
if (${RENDERDOOS_PLATFORM} STREQUAL "win32")
list(APPEND HDRS render_context_gl.h)
list(APPEND SRCS render_context_gl.cpp)
endif (${RENDERDOOS_PLATFORM} STREQUAL "win32")
if (${RENDERDOOS_PLATFORM} STREQUAL "linux")
list(APPEND HDRS render_context_gl.h)
list(APPEND SRCS render_context_gl.cpp)
endif (${RENDERDOOS_PLATFORM} STREQUAL "linux")


if (WIN32)
set(CMAKE_C_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_CXX_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_C_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi")
set(CMAKE_CXX_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi")
endif(WIN32)

if (UNIX)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pthread -std=c++11")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -pthread -std=c++11")
endif (UNIX)


add_library(RenderDoos STATIC ${HDRS} ${SRCS} ${SHADERS})

if (WIN32)
target_compile_definitions(RenderDoos PRIVATE _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS NOMINMAX _UNICODE UNICODE MEMORY_LEAK_TRACKING)
endif (WIN32)

if (${RENDERDOOS_ARCHITECTURE} STREQUAL "arm")
target_compile_definitions(RenderDoos PUBLIC RENDERDOOS_ARM)
else (${RENDERDOOS_ARCHITECTURE} STREQUAL "arm")
target_compile_definitions(RenderDoos PUBLIC RENDERDOOS_X64L)
endif (${RENDERDOOS_ARCHITECTURE} STREQUAL "arm")

if (${RENDERDOOS_PLATFORM} STREQUAL "ios")
target_compile_definitions(RenderDoos PUBLIC RENDERDOOS_METAL)
endif (${RENDERDOOS_PLATFORM} STREQUAL "ios")
if (${RENDERDOOS_PLATFORM} STREQUAL "macos")
target_compile_definitions(RenderDoos PUBLIC RENDERDOOS_METAL)
endif (${RENDERDOOS_PLATFORM} STREQUAL "macos")
if (${RENDERDOOS_PLATFORM} STREQUAL "win32")
target_compile_definitions(RenderDoos PUBLIC RENDERDOOS_OPENGL)
endif (${RENDERDOOS_PLATFORM} STREQUAL "win32")
if (${RENDERDOOS_PLATFORM} STREQUAL "linux")
target_compile_definitions(RenderDoos PUBLIC RENDERDOOS_OPENGL)
endif (${RENDERDOOS_PLATFORM} STREQUAL "linux")



if (RENDERDOOS_SIMD)
target_compile_definitions(RenderDoos PUBLIC RENDERDOOS_SIMD)
endif (RENDERDOOS_SIMD)


source_group("Header Files" FILES ${hdrs})
source_group("Source Files" FILES ${srcs})

if (APPLE)
set_source_files_properties(${SHADERS} PROPERTIES LANGUAGE METAL)
endif (APPLE)

target_include_directories(RenderDoos
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../glew/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../glew/include/glew
  )