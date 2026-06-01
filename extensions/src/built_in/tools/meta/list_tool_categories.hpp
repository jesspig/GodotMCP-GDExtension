// @tool register
// @source meta
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class ListToolCategoriesTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "list_tool_categories"; }
    String category() const override { return "meta"; }
    String brief() const override { return "列出所有工具分类及各分类下的工具数量"; }
    String description() const override {
        return "返回所有已注册工具的分类列表，每个分类包含名称和工具数量。"
               "用于渐进式工具发现流程的第一步。";
    }
    Dictionary input_schema() const override { return Dictionary(); }
    String source() const override { return "meta"; }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }
        Dictionary data;
        data["categories"] = reg_->get_categories();
        return ToolResult::ok(data);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
