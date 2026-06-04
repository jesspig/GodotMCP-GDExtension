// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class GetToolsTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "get_tools"; }
    String category() const override { return "meta_tools"; }
    String brief() const override { return String::utf8("列出指定分类路径下的所有工具"); }
    String description() const override {
        return String::utf8("根据分类路径返回该分类下所有工具的简要信息列表（id、name、description），"
                            "不包含子分类的工具。");
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        Dictionary cat;
        cat["type"] = "string";
        cat["description"] = String::utf8("分类路径，如 meta_tools、node_tools/property/CanvasItem");
        props["category"] = cat;
        schema["properties"] = props;
        Array req;
        req.push_back("category");
        schema["required"] = req;
        return schema;
    }
    bool is_meta() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }
        const String category = ctx.args.get("category", "");
        if (category.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: category");
        }

        const Array entries = reg_->get_tools_in_category(category);
        Array tools;
        for (int i = 0; i < entries.size(); ++i) {
            Dictionary entry = entries[i];
            Dictionary t;
            t["id"] = entry.get("name", "");
            t["name"] = entry.get("brief", "");
            t["description"] = entry.get("description", "");
            tools.push_back(t);
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
