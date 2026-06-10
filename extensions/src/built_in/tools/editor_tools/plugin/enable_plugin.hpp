// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class EnablePluginTool : public ITool {
public:
    String name() const override { return "enable_plugin"; }
    String category() const override { return "editor_tools/plugin"; }
    String brief() const override {
        return "Enable an editor plugin";
    }
    String description() const override {
        return "Enable a plugin by its addons path "
               "(e.g. \"res://addons/my_plugin\").";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Plugin name to enable";
            props["plugin_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the plugin in addons/";
            props["plugin_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("plugin_name");
            req.append("plugin_path");
            s["required"] = req;
        }
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        String plugin_name = args_string(ctx.args, "plugin_name");
        String plugin_path = args_string(ctx.args, "plugin_path");

        if (plugin_name.is_empty() || plugin_path.is_empty()) {
            return ToolResult::err("BAD_PARAM",
                "Both plugin_name and plugin_path are required");
        }

        if (plugin_path == "res://addons/godot_mcp") {
            return ToolResult::err("SELF_ENABLE",
                "Cannot enable/disable the GodotMCP plugin itself via MCP tools");
        }

        ei->call("set_plugin_enabled", plugin_path, true);

        Dictionary data;
        data["plugin_name"] = plugin_name;
        data["plugin_path"] = plugin_path;
        data["enabled"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
