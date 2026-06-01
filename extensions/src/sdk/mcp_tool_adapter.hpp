#pragma once

#include "built_in/tool_base.hpp"
#include "sdk/mcp_tool_definition.hpp"

namespace godot_mcp {

// ── McpToolDefinitionAdapter ──
// 将 SDK 自定义工具（McpToolDefinition）适配为 ITool 接口，
// 使自定义工具与内置工具共享同一套注册、调度、分类体系。
class McpToolDefinitionAdapter : public ITool {
public:
    McpToolDefinitionAdapter(Ref<McpToolDefinition> def)
        : def_(def) {}

    godot::String name() const override { return def_->get_tool_name(); }
    godot::String category() const override { return def_->get_category(); }

    godot::String category_label() const override {
        // SDK 工具可通过 category_label 属性自定义分组展示名
        if (def_->has_method("get_category_label")) {
            return def_->call("get_category_label");
        }
        return def_->get_category();
    }

    godot::String brief() const override { return def_->get_brief(); }
    godot::String description() const override { return def_->get_description(); }
    godot::Dictionary input_schema() const override { return def_->get_input_schema(); }
    godot::String source() const override { return "custom"; }

protected:
    godot::Dictionary execute_impl(const ToolContext &ctx) override {
        return def_->execute(ctx.args);
    }

private:
    Ref<McpToolDefinition> def_;
};

} // namespace godot_mcp
