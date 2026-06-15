
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetPhysicsStatsTool : public ITool {
public:
    String name() const noexcept override { return "get_physics_stats"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Get physics engine statistics"); }

    Dictionary build_input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        auto *p = godot::Performance::get_singleton();
        Dictionary d;
        d["physics_2d_active_objects"] = p->get_monitor(godot::Performance::PHYSICS_2D_ACTIVE_OBJECTS);
        d["physics_2d_collision_pairs"] = p->get_monitor(godot::Performance::PHYSICS_2D_COLLISION_PAIRS);
        d["physics_2d_island_count"] = p->get_monitor(godot::Performance::PHYSICS_2D_ISLAND_COUNT);
        d["physics_3d_active_objects"] = p->get_monitor(godot::Performance::PHYSICS_3D_ACTIVE_OBJECTS);
        d["physics_3d_collision_pairs"] = p->get_monitor(godot::Performance::PHYSICS_3D_COLLISION_PAIRS);
        d["physics_3d_island_count"] = p->get_monitor(godot::Performance::PHYSICS_3D_ISLAND_COUNT);
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
