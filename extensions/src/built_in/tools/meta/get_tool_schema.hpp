// @tool register
// @source meta
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class GetToolSchemaTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "get_tool_schema"; }
    String category() const override { return "meta"; }
    String brief() const override { return String::utf8("获取指定工具的完整 Schema 定义"); }
    String description() const override {
        return String::utf8("返回指定工具的完整元数据信息，包括名称、描述、简介、"
                            "分类、来源和输入参数 Schema。");
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        Dictionary tn;
        tn["type"] = "string";
        tn["description"] = String::utf8("工具名称");
        props["tool_name"] = tn;
        schema["properties"] = props;
        Array req;
        req.push_back("tool_name");
        schema["required"] = req;
        return schema;
    }
    String source() const override { return "meta"; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }
        const String tool_name = ctx.args.get("tool_name", "");
        if (tool_name.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: tool_name");
        }

        const ToolInfo *info = reg_->get_tool_schema(tool_name);
        if (!info) {
            return ToolResult::err("TOOL_NOT_FOUND", String("Tool not found: ") + tool_name);
        }

        Dictionary data;
        data["name"] = info->name;
        data["description"] = info->description;
        data["brief"] = info->brief;
        data["category"] = info->category;
        data["source"] = info->source;
        data["input_schema"] = info->input_schema;
        return ToolResult::ok(data);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
