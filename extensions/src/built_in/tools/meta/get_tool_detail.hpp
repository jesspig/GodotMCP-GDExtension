
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class GetToolDetailTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "get_tool_detail"; }
    String category() const override { return "meta_tools"; }
    String brief() const override { return String("Get full details of a specific tool"); }
    String description() const override {
        return String("Returns complete metadata for the specified tool, including name, id, "
                      "description, parameters, parameter types, return value, return type, "
                      "required parameters, category path, and usage example.");
    }
    Dictionary build_input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        Dictionary tn;
        tn["type"] = "string";
        tn["description"] = String("Tool name, e.g. get_info, get_canvasitem_position");
        props["name"] = tn;
        schema["properties"] = props;
        Array req;
        req.push_back("name");
        schema["required"] = req;
        return schema;
    }
    bool is_meta() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }
        const String tool_name = ctx.args.get("name", "");
        if (tool_name.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: name");
        }

        const ToolInfo *info = reg_->find_tool_info(tool_name);
        if (!info) {
            return ToolResult::err("TOOL_NOT_FOUND", String("Tool not found: ") + tool_name);
        }

        const String &cat_path = info->category;
        const PackedStringArray cat_segments = cat_path.split("/");
        const String cat_id = cat_segments.is_empty() ? cat_path : cat_segments[cat_segments.size() - 1];

        Dictionary data;
        data["id"] = info->name;
        data["name"] = info->brief;
        data["description"] = info->description;

        Dictionary schema = info->input_schema;
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

        // 鐢熸垚浣跨敤绀轰緥锛圡CP 宸ュ叿璋冪敤鏍煎紡锛岀洿鎺ヤ紶鍙傦級
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

        return ToolResult::ok(data);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
