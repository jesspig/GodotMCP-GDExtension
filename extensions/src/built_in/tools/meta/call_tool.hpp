// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class CallToolTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "call_tool"; }
    String category() const override { return "meta"; }
    String brief() const override { return String::utf8("兜底调用任意已注册工具（不推荐直接使用）"); }
    String category_description() const override { return String::utf8("元工具与系统信息查询"); }
    String description() const override {
        return String::utf8("通过工具名称调用任意已注册工具。AI 客户端应优先使用原生工具调用，"
                            "仅在无法直接调用时使用此兜底方法。");
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        Dictionary tn;
        tn["type"] = "string";
        tn["description"] = String::utf8("要调用的工具名称");
        props["tool_name"] = tn;
        Dictionary args;
        args["type"] = "object";
        args["description"] = String::utf8("工具参数（可选的键值对）");
        props["arguments"] = args;
        schema["properties"] = props;
        Array req;
        req.push_back("tool_name");
        schema["required"] = req;
        return schema;
    }
    bool is_meta() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }
        const String tool_name = ctx.args.get("tool_name", "");
        if (tool_name.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: tool_name");
        }

        Dictionary tool_args;
        const Variant arguments = ctx.args.get("arguments", Variant());
        if (arguments.get_type() == Variant::DICTIONARY) {
            tool_args = Dictionary(arguments);
        }

        return reg_->execute(tool_name, tool_args);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
