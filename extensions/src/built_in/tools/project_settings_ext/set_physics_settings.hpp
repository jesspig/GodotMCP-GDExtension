// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class SetPhysicsSettingsTool : public ITool {
public:
    String name() const override { return "set_physics_settings"; }
    String category() const override { return "project_settings_ext"; }
    String brief() const override { return "Set physics settings"; }
    String description() const override { return "Set physics settings"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["gravity_2d"] = [](){Dictionary d;d["type"]="number";return d;}();
        p["gravity_vector_2d"] = [](){Dictionary d;d["type"]="object";return d;}();
        p["linear_damp_2d"] = [](){Dictionary d;d["type"]="number";return d;}();
        p["angular_damp_2d"] = [](){Dictionary d;d["type"]="number";return d;}();
        p["gravity_3d"] = [](){Dictionary d;d["type"]="number";return d;}();
        p["gravity_vector_3d"] = [](){Dictionary d;d["type"]="object";return d;}();
        p["linear_damp_3d"] = [](){Dictionary d;d["type"]="number";return d;}();
        p["angular_damp_3d"] = [](){Dictionary d;d["type"]="number";return d;}();
        p["physics_ticks_per_second"] = [](){Dictionary d;d["type"]="integer";return d;}();
        p["max_physics_steps_per_frame"] = [](){Dictionary d;d["type"]="integer";return d;}();
        s["properties"] = p; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override { return ToolResult::ok(apply_and_save(ctx.args, physics_keys())); }
};

} // namespace godot_mcp
