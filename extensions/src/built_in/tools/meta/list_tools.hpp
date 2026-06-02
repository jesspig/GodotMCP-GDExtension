// @tool register
// @source meta
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class ListToolsTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "list_tools"; }
    String category() const override { return "meta"; }
    String brief() const override { return String::utf8("列出指定分类下的所有工具（名称 + 简介）"); }
    String description() const override {
        return String::utf8("根据分类名称返回该分类下所有工具的简短信息列表（name + brief），"
                            "支持通过 filter 参数进行模糊搜索。");
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        Dictionary cat;
        cat["type"] = "string";
        cat["description"] = String::utf8("分类名称");
        props["category"] = cat;
        Dictionary flt;
        flt["type"] = "string";
        flt["description"] = String::utf8("可选的名称过滤关键字");
        props["filter"] = flt;
        schema["properties"] = props;
        Array req;
        req.push_back("category");
        schema["required"] = req;
        return schema;
    }
    String source() const override { return "meta"; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }
        const String category = ctx.args.get("category", "");
        if (category.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: category");
        }

        const String filter = ctx.args.get("filter", "");
        Array tools = reg_->get_tools_in_category(category);

        if (!filter.is_empty()) {
            Array filtered;
            for (int i = 0; i < tools.size(); ++i) {
                Dictionary t = tools[i];
                const String name = t.get("name", "");
                const String brief = t.get("brief", "");
                if (name.findn(filter) >= 0 || brief.findn(filter) >= 0) {
                    filtered.push_back(t);
                }
            }
            tools = filtered;
        }

        Dictionary data;
        data["category"] = category;
        data["tools"] = tools;
        data["count"] = tools.size();
        return ToolResult::ok(data);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
