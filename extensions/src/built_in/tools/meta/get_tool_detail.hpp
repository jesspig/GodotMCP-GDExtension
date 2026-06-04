// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class GetToolDetailTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "get_tool_detail"; }
    String category() const override { return "meta_tools"; }
    String brief() const override { return String::utf8("获取指定工具的完整信息"); }
    String description() const override {
        return String::utf8("返回指定工具的完整元数据，包括名称、id、描述、入参、"
                            "入参类型、返回值、返回类型、必填参数、分类路径、使用示例等。");
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        Dictionary tn;
        tn["type"] = "string";
        tn["description"] = String::utf8("工具名称，如 get_info、get_canvasitem_position");
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

        const ToolInfo *info = reg_->get_tool_schema(tool_name);
        if (!info) {
            return ToolResult::err("TOOL_NOT_FOUND", String("Tool not found: ") + tool_name);
        }

        const String &cat_path = info->category;

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
        ret["description"] = String::utf8("工具执行结果，包含 success 标志和 data 或 error 字段");
        data["return_value"] = ret;

        data["category_id"] = info->category;
        data["category_path"] = cat_path;

        // 生成使用示例
        String example = String("{\n  \"tool_name\": \"") + tool_name + String("\",\n  \"arguments\": {");
        for (int i = 0; i < param_names.size(); ++i) {
            String pn = param_names[i];
            Dictionary pdef = props[pn];
            String ptype = pdef.get("type", "string");
            String val;
            if (ptype == "string") val = String("\"<") + pn + String(">\"");
            else if (ptype == "integer" || ptype == "number") val = "0";
            else if (ptype == "boolean") val = "true";
            else val = String("\"<") + pn + String(">\"");
            example += String("\n    \"") + pn + String("\": ") + val;
            if (i < param_names.size() - 1) example += ",";
        }
        example += "\n  }\n}";
        data["usage_example"] = example;

        return ToolResult::ok(data);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
