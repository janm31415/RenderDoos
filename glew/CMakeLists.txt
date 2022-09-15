set(HDRS
include/glew/glew.txt
include/glew/GL/glew.h
include/glew/GL/glxew.h
include/glew/GL/wglew.h
	)
	
set(SRCS
src/dummy.cpp
)

if (WIN32)
set(CMAKE_C_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_CXX_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_C_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi /DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi /DNDEBUG")
endif(WIN32)

add_library(RenderDoosGlew STATIC ${HDRS} ${SRCS})
source_group("Header Files" FILES ${hdrs})
source_group("Source Files" FILES ${srcs})

target_include_directories(RenderDoosGlew
    PRIVATE
    )	
	
target_link_libraries(RenderDoosGlew
    PRIVATE		
    )	