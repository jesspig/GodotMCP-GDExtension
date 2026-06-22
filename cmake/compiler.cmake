# cmake/compiler.cmake
# Compiler-specific flags for godot_mcp_gdext based on CMAKE_CXX_COMPILER_ID
# Applied using generator expressions or per-compiler if() blocks.

if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
    target_compile_options(godot_mcp_gdext PRIVATE /utf-8 /bigobj /W4 /wd4244 /wd4267 /EHsc)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(godot_mcp_gdext PRIVATE -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden)
    if(NOT DEFINED ENV{CI})
        target_compile_options(godot_mcp_gdext PRIVATE -march=native)
    endif()
    if(WIN32)
        target_link_options(godot_mcp_gdext PRIVATE -static-libgcc -static-libstdc++)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(WIN32)
        target_compile_options(godot_mcp_gdext PRIVATE /W4 /wd4244 /wd4267 -Wno-unused-parameter -Wno-unused-private-field /EHsc)
    else()
        target_compile_options(godot_mcp_gdext PRIVATE -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden)
        if(NOT DEFINED ENV{CI})
            target_compile_options(godot_mcp_gdext PRIVATE -march=native)
        endif()
    endif()
endif()

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER})")
