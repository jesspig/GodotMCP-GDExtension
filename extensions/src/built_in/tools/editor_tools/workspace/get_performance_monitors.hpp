// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetPerformanceMonitorsTool : public ITool {
public:
    String name() const override { return "get_performance_monitors"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Get performance monitor data"); }
    String description() const override {
        return String("Reads monitor data from the Godot Performance singleton, including FPS, "
                      "memory usage, object count, render statistics, physics statistics, etc. "
                      "Aligned with the get_monitor() API in Godot source main/performance.h, "
                      "with a total of 59 monitor enums. Supports filtering specific monitors by name.");
    }

    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "array";
            p["description"] = String("List of monitor names to query, leave empty for all");
            p["items"] = Dictionary();  // accepts strings
            props["monitors"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Array requested = ctx.args.has("monitors")
            ? (Array)ctx.args["monitors"]
            : Array();

        Performance *perf = Performance::get_singleton();
        if (!perf) {
            return ToolResult::err("NO_PERF", "Performance singleton not available");
        }

        Dictionary data;
        Dictionary all_monitors;

        struct MonitorEntry {
            String name;
            Performance::Monitor id;
        };

        static const MonitorEntry kAllMonitors[] = {
            { "time/fps",                    Performance::TIME_FPS },
            { "time/process",                Performance::TIME_PROCESS },
            { "time/physics_process",        Performance::TIME_PHYSICS_PROCESS },
            { "time/navigation_process",     Performance::TIME_NAVIGATION_PROCESS },
            { "memory/static",               Performance::MEMORY_STATIC },
            { "memory/static_max",           Performance::MEMORY_STATIC_MAX },
            { "memory/message_buffer_max",   Performance::MEMORY_MESSAGE_BUFFER_MAX },
            { "object/count",                Performance::OBJECT_COUNT },
            { "object/resource_count",       Performance::OBJECT_RESOURCE_COUNT },
            { "object/node_count",           Performance::OBJECT_NODE_COUNT },
            { "object/orphan_node_count",    Performance::OBJECT_ORPHAN_NODE_COUNT },
            { "render/total_objects",        Performance::RENDER_TOTAL_OBJECTS_IN_FRAME },
            { "render/total_primitives",     Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME },
            { "render/total_draw_calls",     Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME },
            { "render/video_mem_used",       Performance::RENDER_VIDEO_MEM_USED },
            { "render/texture_mem_used",     Performance::RENDER_TEXTURE_MEM_USED },
            { "render/buffer_mem_used",      Performance::RENDER_BUFFER_MEM_USED },
            { "physics/2d_active_objects",   Performance::PHYSICS_2D_ACTIVE_OBJECTS },
            { "physics/2d_collision_pairs",  Performance::PHYSICS_2D_COLLISION_PAIRS },
            { "physics/2d_island_count",     Performance::PHYSICS_2D_ISLAND_COUNT },
            { "physics/3d_active_objects",   Performance::PHYSICS_3D_ACTIVE_OBJECTS },
            { "physics/3d_collision_pairs",  Performance::PHYSICS_3D_COLLISION_PAIRS },
            { "physics/3d_island_count",     Performance::PHYSICS_3D_ISLAND_COUNT },
            { "audio/output_latency",        Performance::AUDIO_OUTPUT_LATENCY },
            { "nav/active_maps",             Performance::NAVIGATION_ACTIVE_MAPS },
            { "nav/region_count",            Performance::NAVIGATION_REGION_COUNT },
            { "nav/agent_count",             Performance::NAVIGATION_AGENT_COUNT },
            { "nav/link_count",              Performance::NAVIGATION_LINK_COUNT },
            { "nav/polygon_count",           Performance::NAVIGATION_POLYGON_COUNT },
            { "nav/edge_count",              Performance::NAVIGATION_EDGE_COUNT },
            { "nav/edge_merge_count",        Performance::NAVIGATION_EDGE_MERGE_COUNT },
            { "nav/edge_connection_count",   Performance::NAVIGATION_EDGE_CONNECTION_COUNT },
            { "nav/edge_free_count",         Performance::NAVIGATION_EDGE_FREE_COUNT },
            { "nav/obstacle_count",          Performance::NAVIGATION_OBSTACLE_COUNT },
            { "pipeline/compilations_canvas", Performance::PIPELINE_COMPILATIONS_CANVAS },
            { "pipeline/compilations_mesh",  Performance::PIPELINE_COMPILATIONS_MESH },
            { "pipeline/compilations_surface", Performance::PIPELINE_COMPILATIONS_SURFACE },
            { "pipeline/compilations_draw",  Performance::PIPELINE_COMPILATIONS_DRAW },
            { "pipeline/compilations_specialization", Performance::PIPELINE_COMPILATIONS_SPECIALIZATION },
            { "nav/2d_active_maps",          Performance::NAVIGATION_2D_ACTIVE_MAPS },
            { "nav/2d_region_count",         Performance::NAVIGATION_2D_REGION_COUNT },
            { "nav/2d_agent_count",          Performance::NAVIGATION_2D_AGENT_COUNT },
            { "nav/2d_link_count",           Performance::NAVIGATION_2D_LINK_COUNT },
            { "nav/2d_polygon_count",        Performance::NAVIGATION_2D_POLYGON_COUNT },
            { "nav/2d_edge_count",           Performance::NAVIGATION_2D_EDGE_COUNT },
            { "nav/2d_edge_merge_count",     Performance::NAVIGATION_2D_EDGE_MERGE_COUNT },
            { "nav/2d_edge_connection_count", Performance::NAVIGATION_2D_EDGE_CONNECTION_COUNT },
            { "nav/2d_edge_free_count",      Performance::NAVIGATION_2D_EDGE_FREE_COUNT },
            { "nav/2d_obstacle_count",       Performance::NAVIGATION_2D_OBSTACLE_COUNT },
            { "nav/3d_active_maps",          Performance::NAVIGATION_3D_ACTIVE_MAPS },
            { "nav/3d_region_count",         Performance::NAVIGATION_3D_REGION_COUNT },
            { "nav/3d_agent_count",          Performance::NAVIGATION_3D_AGENT_COUNT },
            { "nav/3d_link_count",           Performance::NAVIGATION_3D_LINK_COUNT },
            { "nav/3d_polygon_count",        Performance::NAVIGATION_3D_POLYGON_COUNT },
            { "nav/3d_edge_count",           Performance::NAVIGATION_3D_EDGE_COUNT },
            { "nav/3d_edge_merge_count",     Performance::NAVIGATION_3D_EDGE_MERGE_COUNT },
            { "nav/3d_edge_connection_count", Performance::NAVIGATION_3D_EDGE_CONNECTION_COUNT },
            { "nav/3d_edge_free_count",      Performance::NAVIGATION_3D_EDGE_FREE_COUNT },
            { "nav/3d_obstacle_count",       Performance::NAVIGATION_3D_OBSTACLE_COUNT },
        };

        constexpr int64_t kMonitorCount = sizeof(kAllMonitors) / sizeof(kAllMonitors[0]);

        if (requested.size() > 0) {
            for (int64_t i = 0; i < requested.size(); i++) {
                String key = requested[i].stringify().strip_edges().to_lower();
                for (int64_t j = 0; j < kMonitorCount; j++) {
                    if (kAllMonitors[j].name == key) {
                        all_monitors[key] = perf->get_monitor(kAllMonitors[j].id);
                        break;
                    }
                }
            }
        } else {
            for (int64_t j = 0; j < kMonitorCount; j++) {
                all_monitors[kAllMonitors[j].name] = perf->get_monitor(kAllMonitors[j].id);
            }
        }

        data["monitors"] = all_monitors;

        bool is_running = Engine::get_singleton()->is_editor_hint() == false;
        data["is_running"] = is_running;

        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
