
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"
#include "built_in/cmd_utils/dispatch_map.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetPerformanceMonitorsTool : public ITool {
public:
    String name() const noexcept override { return "get_performance_monitors"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Get performance monitor data"); }
    String description() const override {
        return String("Reads monitor data from the Godot Performance singleton, including FPS, "
                      "memory usage, object count, render statistics, physics statistics, etc. "
                      "Aligned with the get_monitor() API in Godot source main/performance.h, "
                      "with a total of 59 monitor enums. Supports filtering specific monitors by name.");
    }

    Dictionary build_input_schema() const override {
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
        Array requested = args_get_typed<Array>(ctx.args, "monitors", Array());

        auto *perf = godot::Performance::get_singleton();
        if (!perf) {
            return ToolResult::err("NO_PERF", "Performance singleton not available");
        }

        Dictionary data;
        Dictionary all_monitors;

        static const auto kMonitorMap = godot_mcp::make_dispatch_map<godot::String, godot::Performance::Monitor>(
            std::pair{godot::String("time/fps"),                       godot::Performance::TIME_FPS},
            std::pair{godot::String("time/process"),                   godot::Performance::TIME_PROCESS},
            std::pair{godot::String("time/physics_process"),           godot::Performance::TIME_PHYSICS_PROCESS},
            std::pair{godot::String("time/navigation_process"),        godot::Performance::TIME_NAVIGATION_PROCESS},
            std::pair{godot::String("memory/static"),                  godot::Performance::MEMORY_STATIC},
            std::pair{godot::String("memory/static_max"),              godot::Performance::MEMORY_STATIC_MAX},
            std::pair{godot::String("memory/message_buffer_max"),      godot::Performance::MEMORY_MESSAGE_BUFFER_MAX},
            std::pair{godot::String("object/count"),                   godot::Performance::OBJECT_COUNT},
            std::pair{godot::String("object/resource_count"),          godot::Performance::OBJECT_RESOURCE_COUNT},
            std::pair{godot::String("object/node_count"),              godot::Performance::OBJECT_NODE_COUNT},
            std::pair{godot::String("object/orphan_node_count"),       godot::Performance::OBJECT_ORPHAN_NODE_COUNT},
            std::pair{godot::String("render/total_objects"),           godot::Performance::RENDER_TOTAL_OBJECTS_IN_FRAME},
            std::pair{godot::String("render/total_primitives"),        godot::Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME},
            std::pair{godot::String("render/total_draw_calls"),        godot::Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME},
            std::pair{godot::String("render/video_mem_used"),          godot::Performance::RENDER_VIDEO_MEM_USED},
            std::pair{godot::String("render/texture_mem_used"),        godot::Performance::RENDER_TEXTURE_MEM_USED},
            std::pair{godot::String("render/buffer_mem_used"),         godot::Performance::RENDER_BUFFER_MEM_USED},
            std::pair{godot::String("physics/2d_active_objects"),      godot::Performance::PHYSICS_2D_ACTIVE_OBJECTS},
            std::pair{godot::String("physics/2d_collision_pairs"),     godot::Performance::PHYSICS_2D_COLLISION_PAIRS},
            std::pair{godot::String("physics/2d_island_count"),        godot::Performance::PHYSICS_2D_ISLAND_COUNT},
            std::pair{godot::String("physics/3d_active_objects"),      godot::Performance::PHYSICS_3D_ACTIVE_OBJECTS},
            std::pair{godot::String("physics/3d_collision_pairs"),     godot::Performance::PHYSICS_3D_COLLISION_PAIRS},
            std::pair{godot::String("physics/3d_island_count"),        godot::Performance::PHYSICS_3D_ISLAND_COUNT},
            std::pair{godot::String("audio/output_latency"),           godot::Performance::AUDIO_OUTPUT_LATENCY},
            std::pair{godot::String("nav/active_maps"),                godot::Performance::NAVIGATION_ACTIVE_MAPS},
            std::pair{godot::String("nav/region_count"),               godot::Performance::NAVIGATION_REGION_COUNT},
            std::pair{godot::String("nav/agent_count"),                godot::Performance::NAVIGATION_AGENT_COUNT},
            std::pair{godot::String("nav/link_count"),                 godot::Performance::NAVIGATION_LINK_COUNT},
            std::pair{godot::String("nav/polygon_count"),              godot::Performance::NAVIGATION_POLYGON_COUNT},
            std::pair{godot::String("nav/edge_count"),                 godot::Performance::NAVIGATION_EDGE_COUNT},
            std::pair{godot::String("nav/edge_merge_count"),           godot::Performance::NAVIGATION_EDGE_MERGE_COUNT},
            std::pair{godot::String("nav/edge_connection_count"),      godot::Performance::NAVIGATION_EDGE_CONNECTION_COUNT},
            std::pair{godot::String("nav/edge_free_count"),            godot::Performance::NAVIGATION_EDGE_FREE_COUNT},
            std::pair{godot::String("nav/obstacle_count"),             godot::Performance::NAVIGATION_OBSTACLE_COUNT},
            std::pair{godot::String("pipeline/compilations_canvas"),   godot::Performance::PIPELINE_COMPILATIONS_CANVAS},
            std::pair{godot::String("pipeline/compilations_mesh"),     godot::Performance::PIPELINE_COMPILATIONS_MESH},
            std::pair{godot::String("pipeline/compilations_surface"),  godot::Performance::PIPELINE_COMPILATIONS_SURFACE},
            std::pair{godot::String("pipeline/compilations_draw"),     godot::Performance::PIPELINE_COMPILATIONS_DRAW},
            std::pair{godot::String("pipeline/compilations_specialization"), godot::Performance::PIPELINE_COMPILATIONS_SPECIALIZATION},
            std::pair{godot::String("nav/2d_active_maps"),             godot::Performance::NAVIGATION_2D_ACTIVE_MAPS},
            std::pair{godot::String("nav/2d_region_count"),            godot::Performance::NAVIGATION_2D_REGION_COUNT},
            std::pair{godot::String("nav/2d_agent_count"),             godot::Performance::NAVIGATION_2D_AGENT_COUNT},
            std::pair{godot::String("nav/2d_link_count"),              godot::Performance::NAVIGATION_2D_LINK_COUNT},
            std::pair{godot::String("nav/2d_polygon_count"),           godot::Performance::NAVIGATION_2D_POLYGON_COUNT},
            std::pair{godot::String("nav/2d_edge_count"),              godot::Performance::NAVIGATION_2D_EDGE_COUNT},
            std::pair{godot::String("nav/2d_edge_merge_count"),        godot::Performance::NAVIGATION_2D_EDGE_MERGE_COUNT},
            std::pair{godot::String("nav/2d_edge_connection_count"),   godot::Performance::NAVIGATION_2D_EDGE_CONNECTION_COUNT},
            std::pair{godot::String("nav/2d_edge_free_count"),         godot::Performance::NAVIGATION_2D_EDGE_FREE_COUNT},
            std::pair{godot::String("nav/2d_obstacle_count"),          godot::Performance::NAVIGATION_2D_OBSTACLE_COUNT},
            std::pair{godot::String("nav/3d_active_maps"),             godot::Performance::NAVIGATION_3D_ACTIVE_MAPS},
            std::pair{godot::String("nav/3d_region_count"),            godot::Performance::NAVIGATION_3D_REGION_COUNT},
            std::pair{godot::String("nav/3d_agent_count"),             godot::Performance::NAVIGATION_3D_AGENT_COUNT},
            std::pair{godot::String("nav/3d_link_count"),              godot::Performance::NAVIGATION_3D_LINK_COUNT},
            std::pair{godot::String("nav/3d_polygon_count"),           godot::Performance::NAVIGATION_3D_POLYGON_COUNT},
            std::pair{godot::String("nav/3d_edge_count"),              godot::Performance::NAVIGATION_3D_EDGE_COUNT},
            std::pair{godot::String("nav/3d_edge_merge_count"),        godot::Performance::NAVIGATION_3D_EDGE_MERGE_COUNT},
            std::pair{godot::String("nav/3d_edge_connection_count"),   godot::Performance::NAVIGATION_3D_EDGE_CONNECTION_COUNT},
            std::pair{godot::String("nav/3d_edge_free_count"),         godot::Performance::NAVIGATION_3D_EDGE_FREE_COUNT},
            std::pair{godot::String("nav/3d_obstacle_count"),          godot::Performance::NAVIGATION_3D_OBSTACLE_COUNT}
        );
        assert(kMonitorMap.validate() && "Duplicate monitor key");

        if (requested.size() > 0) {
            for (int64_t i = 0; i < requested.size(); i++) {
                String key = requested[i].stringify().strip_edges().to_lower();
                if (const auto *id = kMonitorMap.find(key)) {
                    all_monitors[key] = perf->get_monitor(*id);
                }
            }
        } else {
            for (const auto &kv : kMonitorMap.data()) {
                all_monitors[kv.first] = perf->get_monitor(kv.second);
            }
        }

        data["monitors"] = all_monitors;

        bool is_running = false;
        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            is_running = ei->is_playing_scene();
        }
        data["is_running"] = is_running;

        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
