// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class ExportProjectTool : public ITool {
public:
    String name() const override { return "export_project"; }
    String category() const override { return "editor_tools/export"; }
    String brief() const override {
        return "Export the project using a named preset";
    }
    String description() const override {
        return "Export the current project using a configured export preset. "
               "Optionally specify an output path for the exported file.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Export preset name to use";
            props["preset_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Output path for the exported file (optional)";
            props["output_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("preset_name");
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

        String preset_name = args_string(ctx.args, "preset_name");
        String output_path = args_string(ctx.args, "output_path");

        if (preset_name.is_empty()) {
            return ToolResult::err("BAD_PARAM", "preset_name is required");
        }

        Dictionary export_args;
        export_args["preset_name"] = preset_name;
        if (!output_path.is_empty()) {
            export_args["output_path"] = output_path;
        }

        Variant result = ei->call("export_project", preset_name, output_path);

        Dictionary data;
        data["exported"] = true;
        data["preset_name"] = preset_name;
        data["output_path"] = output_path.is_empty() ? Variant() : Variant(output_path);
        if (result.get_type() != Variant::NIL) {
            data["result"] = result;
        }
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
