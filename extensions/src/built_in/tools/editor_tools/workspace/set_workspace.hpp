#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class SetWorkspaceTool : public ITool {
public:
    String name() const override { return "set_workspace"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Switch editor workspace"); }
    String description() const override {
        return String("Switches to the specified workspace: 2D (2D editing), 3D (3D editing), "
                      "Script (script editing), AssetLib (asset library). "
                      "Aligned with the select_by_name() flow in Godot source "
                      "editor/editor_main_screen.cpp, implemented via "
                      "EditorInterface::set_main_screen_editor().");
    }

    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Target workspace name: 2D / 3D / Script / AssetLib");
            props["name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("name");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String name = args_string(ctx.args, "name");
        if (name.is_empty()) {
            return ToolResult::err("MISSING_ARG", "name cannot be empty, valid values: 2D / 3D / Script / AssetLib");
        }

        String normalized = name.strip_edges().capitalize();
        static const PackedStringArray valid_names = { "2D", "3D", "Script", "AssetLib" };
        bool valid = false;
        for (int i = 0; i < valid_names.size(); i++) {
            if (normalized == valid_names[i]) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            return ToolResult::err("INVALID_ARG",
                String("Invalid workspace name '") + name + String("', valid values: 2D / 3D / Script / AssetLib"));
        }

        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        ei->set_main_screen_editor(normalized);

        Dictionary data;
        data["workspace"] = normalized;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
