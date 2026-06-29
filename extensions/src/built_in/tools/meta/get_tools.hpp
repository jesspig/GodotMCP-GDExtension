
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class GetToolsTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const noexcept override { return "get_tools"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return String("Get a tool's full schema by name, or list all tools"); }
    String description() const override {
        return String("Two modes:\n"
                      "- With name: returns the full schema for that specific tool (parameters, types, descriptions, usage example).\n"
                      "- Without name: returns a brief list of all registered tools (name, brief description, category).\n"
                      "Use get_categories to browse the category tree, and find_tool to search by keyword.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("name", "string", "Tool name (e.g. add_node, get_game_scene_tree). Returns full detail for that tool.")
            .build();
    }
    bool is_meta() const noexcept override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }

        const String tool_name = ctx.args.get("name", "");

        // name provided → return that tool's full detail
        if (!tool_name.is_empty()) {
            const ToolInfo *info = reg_->find_tool_info(tool_name);
            if (!info) {
                return ToolResult::err("TOOL_NOT_FOUND", String("Tool not found: ") + tool_name);
            }
            return ToolResult::ok(build_tool_detail(tool_name, info));
        }

        // no name → list all tools (brief)
        Array all_tools;
        PackedStringArray categories = reg_->get_all_category_paths();
        for (int i = 0; i < categories.size(); ++i) {
            const Array entries = reg_->get_tools_in_category(categories[i]);
            for (int j = 0; j < entries.size(); ++j) {
                Dictionary entry = entries[j];
                Dictionary t;
                t["id"] = entry.get("name", "");
                t["name"] = entry.get("brief", "");
                t["description"] = entry.get("description", "");
                t["category"] = categories[i];
                all_tools.push_back(t);
            }
        }
        Dictionary data;
        data["tools"] = all_tools;
        data["count"] = all_tools.size();
        return ToolResult::ok(data);
    }

private:
    static Dictionary build_tool_detail(const String &tool_name, const ToolInfo *info) {
        const String cat_path = info->tool_ptr->category();
        const PackedStringArray cat_segments = cat_path.split("/");
        const String cat_id = cat_segments.is_empty() ? cat_path : cat_segments[cat_segments.size() - 1];

        Dictionary data;
        data["id"] = tool_name;
        data["name"] = info->tool_ptr->brief();
        data["description"] = info->tool_ptr->description();

        Dictionary schema = info->tool_ptr->input_schema();
        Dictionary props = schema.get("properties", Dictionary());
        Array param_names = props.keys();
        Array params;
        for (int i = 0; i < param_names.size(); ++i) {
            Dictionary param;
            String pname = param_names[i];
            Dictionary pdef = props[pname];
            param["name"] = pname;
            param["type"] = pdef.get("type", "string");
            param["description"] = pdef.get("description", "");
            params.push_back(param);
        }
        data["parameters"] = params;

        Array required = schema.get("required", Array());
        data["required"] = required;

        Dictionary ret;
        ret["type"] = "object";
        ret["description"] = String("Tool execution result containing a success flag and either data or error fields");
        data["return_value"] = ret;

        data["category_id"] = cat_id;
        data["category_path"] = cat_path;

        String example = String("{\n");
        for (int i = 0; i < param_names.size(); ++i) {
            String pn = param_names[i];
            Dictionary pdef = props[pn];
            String ptype = pdef.get("type", "string");
            String val;
            if (ptype == "string") val = String("\"<") + pn + String(">\"");
            else if (ptype == "integer" || ptype == "number") val = String("<") + pn + String(">");
            else if (ptype == "boolean") val = "true";
            else val = String("\"<") + pn + String(">\"");
            example += String("  \"") + pn + String("\": ") + val;
            if (i < param_names.size() - 1) example += ",";
            example += "\n";
        }
        example += "}";
        data["usage_example"] = example;

        return data;
    }

    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
