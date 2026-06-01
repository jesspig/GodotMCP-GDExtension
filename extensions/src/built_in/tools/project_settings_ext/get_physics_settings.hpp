// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class GetPhysicsSettingsTool : public ITool {
public:
    String name() const override { return "get_physics_settings"; }
    String category() const override { return "project_settings_ext"; }
    String brief() const override { return "Get physics settings"; }
    String description() const override { return "Get physics settings"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override { return ToolResult::ok(gather(physics_keys())); }
};

} // namespace godot_mcp
