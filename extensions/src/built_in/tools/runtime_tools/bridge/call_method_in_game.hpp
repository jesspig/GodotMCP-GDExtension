
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class CallMethodInGameTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "call_method_in_game"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Call a method on a running game node"); }
    String description() const override {
        return String("Calls an arbitrary method on a node in the running game and returns the result. "
                             "node_path is the node path (e.g. /root/Main/Player), "
                             "method is the method name, args is the method argument array.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path, e.g. /root/Main/Player")
            .prop("method", "string", "Method name")
            .prop("args", "array", "Method argument array (optional)")
            .prop("timeout_ms", "integer", "Response timeout in ms", (int64_t)100)
            .required({"node_path", "method"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", "Game not running");
        }
        Dictionary params;
        params["node_path"] = args_string(ctx.args, "node_path");
        params["method"] = args_string(ctx.args, "method");
        params["args"] = ctx.args.get("args", Array());
        int timeout = args_int(ctx.args, "timeout_ms", 100);
        return RuntimeBridge::make_response(bridge->send_command("call_method", params, timeout));
    }
};

} // namespace godot_mcp

