# cmake/cache.cmake
# Compiler cache: sccache / ccache auto-detection.
# sccache supports MSVC and GCC/Clang; ccache supports GCC/Clang only.

set(GODOTMCP_CACHE "auto" CACHE STRING "Compiler cache: auto, sccache, ccache, off")
set_property(CACHE GODOTMCP_CACHE PROPERTY STRINGS auto sccache ccache off)

if(GODOTMCP_CACHE STREQUAL "off")
    message(STATUS "Compiler cache: disabled (GODOTMCP_CACHE=off)")
    return()
endif()

find_program(GODOTMCP_SCCACHE NAMES sccache)
find_program(GODOTMCP_CCACHE NAMES ccache)

set(_cache_program "")

if(GODOTMCP_CACHE STREQUAL "sccache")
    if(GODOTMCP_SCCACHE)
        set(_cache_program "${GODOTMCP_SCCACHE}")
    else()
        message(WARNING "sccache requested but not found")
    endif()
elseif(GODOTMCP_CACHE STREQUAL "ccache")
    if(MSVC)
        message(WARNING "ccache does not support MSVC; skipping")
    elseif(GODOTMCP_CCACHE)
        set(_cache_program "${GODOTMCP_CCACHE}")
    else()
        message(WARNING "ccache requested but not found")
    endif()
else()
    if(GODOTMCP_SCCACHE)
        set(_cache_program "${GODOTMCP_SCCACHE}")
    elseif(NOT MSVC AND GODOTMCP_CCACHE)
        set(_cache_program "${GODOTMCP_CCACHE}")
    endif()
endif()

if(_cache_program)
    set(CMAKE_C_COMPILER_LAUNCHER "${_cache_program}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${_cache_program}")
    get_filename_component(_cache_name "${_cache_program}" NAME)
    message(STATUS "Compiler cache: ${_cache_name} (${_cache_program})")
else()
    message(STATUS "Compiler cache: not found (direct compilation)")
endif()
