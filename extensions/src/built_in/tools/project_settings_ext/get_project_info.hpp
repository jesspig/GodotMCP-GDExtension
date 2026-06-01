// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class GetProjectInfoTool : public ITool {
public:
    String name() const override { return "get_project_info"; }
    String category() const override { return "project_settings_ext"; }
    String brief() const override { return "Get project info (name, desc, version, icon)"; }
    String description() const override { return "Get project info (name, desc, version, icon)"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override { return ToolResult::ok(gather(project_info_keys())); }
};

} // namespace godot_mcp
