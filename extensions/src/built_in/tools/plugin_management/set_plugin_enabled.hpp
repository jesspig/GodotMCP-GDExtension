// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SetPluginEnabledTool : public ITool {
public:
    String name() const override { return "set_plugin_enabled"; }
    String category() const override { return "plugin_management"; }
    String brief() const override { return "Enable or disable an editor plugin"; }
    String description() const override { return "Enable or disable an editor plugin"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["plugin"] = [](){Dictionary d;d["type"]="string";d["description"]="Plugin directory name";return d;}();
        p["enabled"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Enable or disable";return d;}();
        s["properties"] = p; Array r; r.push_back("plugin"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String plugin = args_string(ctx.args, "plugin");
        if (plugin.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'plugin'");
        bool enabled = args_bool(ctx.args, "enabled", true);
        EditorInterface *ei = EditorInterface::get_singleton();
        bool was = ei->is_plugin_enabled(plugin);
        ei->set_plugin_enabled(plugin, enabled);
        Dictionary d; d["plugin"] = plugin; d["enabled"] = enabled; d["previous"] = was;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
