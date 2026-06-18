# cmake/platform.cmake
# Platform detection: architecture, CI environment.

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(GODOTMCP_ARCH "x86_64")
else()
    set(GODOTMCP_ARCH "x86")
endif()

if(DEFINED ENV{CI})
    set(GODOTMCP_IS_CI ON)
    message(STATUS "CI environment detected")
else()
    set(GODOTMCP_IS_CI OFF)
endif()

message(STATUS "Architecture: ${GODOTMCP_ARCH}")
