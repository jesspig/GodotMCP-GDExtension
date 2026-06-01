// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class GetDisplaySettingsTool : public ITool {
public:
    String name() const override { return "get_display_settings"; }
    String category() const override { return "project_settings_ext"; }
    String brief() const override { return "Get display/window settings"; }
    String description() const override { return "Get display/window settings"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override { return ToolResult::ok(gather(display_keys())); }
};

} // namespace godot_mcp
