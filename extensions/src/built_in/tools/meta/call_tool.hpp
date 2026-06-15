
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class CallToolTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const noexcept override { return "call_tool"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return String("Fallback to call any registered tool (not recommended for direct use)"); }
    String category_description() const override { return String("Meta tools and system information queries"); }
    String description() const override {
        return String("Calls any registered tool by name. AI clients should prefer native tool calls "
                      "and only use this fallback when direct invocation is unavailable.");
    }
    Dictionary build_input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        Dictionary tn;
        tn["type"] = "string";
        tn["description"] = String("Name of the tool to call");
        props["tool_name"] = tn;
        Dictionary args;
        args["type"] = "object";
        args["description"] = String("Tool arguments (optional key-value pairs)");
        props["arguments"] = args;
        schema["properties"] = props;
        Array req;
        req.push_back("tool_name");
        schema["required"] = req;
        return schema;
    }
    bool is_meta() const noexcept override { return true; }

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
