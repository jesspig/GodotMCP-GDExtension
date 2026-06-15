
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SetPluginEnabledTool : public ITool {
public:
    String name() const noexcept override { return "set_plugin_enabled"; }
    String category() const noexcept override { return "editor_tools/plugin"; }
    String brief() const noexcept override {
        return "Enable or disable an editor plugin";
    }
    String description() const override {
        return "Enable or disable a plugin by its addons path "
               "(e.g. \"res://addons/my_plugin\").";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Plugin name to enable/disable";
            props["plugin_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the plugin in addons/";
            props["plugin_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "true = enable plugin, false = disable plugin";
            p["default"] = true;
            props["enabled"] = p;
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
        auto *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        String plugin_name = args_string(ctx.args, "plugin_name");
        String plugin_path = args_string(ctx.args, "plugin_path");

        if (plugin_name.is_empty() || plugin_path.is_empty()) {
            return ToolResult::err("BAD_PARAM",
                "Both plugin_name and plugin_path are required");
        }

        if (plugin_path == String(g_mcp_self_plugin_path.c_str())) {
            return ToolResult::err("SELF_MODIFY",
                "Cannot enable/disable the GodotMCP plugin itself via MCP tools");
        }

        bool enabled = args_bool(ctx.args, "enabled", true);
        ei->call("set_plugin_enabled", plugin_path, enabled);

        Dictionary data;
        data["plugin_name"] = plugin_name;
        data["plugin_path"] = plugin_path;
        data["enabled"] = enabled;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
