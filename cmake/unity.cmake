# cmake/unity.cmake
# Unity (jumbo) build — faster full rebuilds by batching translation units.

option(GODOTMCP_UNITY_BUILD "Enable unity (jumbo) build" ON)
set(GODOTMCP_UNITY_BATCH_SIZE "0" CACHE STRING
    "Unity batch count (0 = adaptive: min(cpu, ceil(files/4)))")

set(_unity_batch ${GODOTMCP_UNITY_BATCH_SIZE})
set(_unity_enabled ${GODOTMCP_UNITY_BUILD})
set(_unity_files 0)

if(_unity_batch STREQUAL "0")
    include(ProcessorCount)
    ProcessorCount(_cpu_count)

    list(LENGTH GODOT_MCP_SOURCES _src_count)
    math(EXPR _unity_files "${_src_count} - 1")

    if(_cpu_count LESS 2)
        set(_unity_batch 1)
        set(_unity_enabled OFF)
        message(STATUS "Unity build disabled (1 core)")
    else()
        math(EXPR _max_batches "(${_unity_files} + 3) / 4")
        if(_max_batches LESS 2)
            set(_max_batches 2)
        endif()
        if(_cpu_count GREATER _max_batches)
            set(_cpu_count ${_max_batches})
        endif()
        set(_unity_batch ${_cpu_count})
        message(STATUS "Unity batch: ${_unity_batch} (${_unity_files} files, max ${_max_batches} batches)")
    endif()
else()
    message(STATUS "Unity batch: ${_unity_batch} (user-specified)")
endif()

set_target_properties(godot_mcp_gdext PROPERTIES
    UNITY_BUILD ${_unity_enabled}
    UNITY_BUILD_BATCH_SIZE ${_unity_batch}
)
