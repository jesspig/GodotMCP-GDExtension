# cmake/lto.cmake
# LTO (Link-Time Optimization) — Release builds only.
#   auto  (default) → Clang ThinLTO if available, else standard IPO
#   thin             → Clang ThinLTO (falls back to IPO on MSVC/GCC)
#   full             → Standard CMake IPO (full LTO)

set(GODOTMCP_LTO_MODE "auto" CACHE STRING
    "LTO mode: auto (detect best), thin (ThinLTO), full (Full LTO)")
set_property(CACHE GODOTMCP_LTO_MODE PROPERTY STRINGS auto thin full)

set(GODOTMCP_LTO "OFF" CACHE INTERNAL "")
include(CheckIPOSupported)

macro(_lto_ipo_fallback)
    check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_err)
    if(_ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(GODOTMCP_LTO "ON (full)" CACHE INTERNAL "")
        message(STATUS "LTO: full (via IPO, Release only)")
    else()
        message(${ARGV0} "LTO not supported: ${_ipo_err}")
    endif()
endmacro()

if((GODOTMCP_LTO_MODE STREQUAL "auto" OR GODOTMCP_LTO_MODE STREQUAL "thin")
    AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options($<$<CONFIG:Release>:-flto=thin>)
    add_link_options($<$<CONFIG:Release>:-flto=thin>)
    set(GODOTMCP_LTO "ON (thin)" CACHE INTERNAL "")
    message(STATUS "LTO: thin (Clang ThinLTO, Release only)")
elseif(GODOTMCP_LTO_MODE STREQUAL "thin")
    _lto_ipo_fallback(WARNING "LTO mode 'thin' not available with this compiler")
else()
    _lto_ipo_fallback(STATUS)
endif()
