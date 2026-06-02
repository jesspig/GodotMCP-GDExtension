// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class GetRenderingSettingsTool : public ITool {
public:
    String name() const override { return "get_rendering_settings"; }
    String category() const override { return "project_settings_ext"; }
    String brief() const override { return "Get rendering settings"; }
    String description() const override { return "Get rendering settings"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override { return ToolResult::ok(gather(rendering_keys())); }
};

} // namespace godot_mcp
