// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class CallMethodInGameTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const override { return "call_method_in_game"; }
    String category() const override { return "runtime_tools/bridge"; }
    String brief() const override { return String("Call a method on a running game node"); }
    String description() const override {
        return String("Calls an arbitrary method on a node in the running game and returns the result. "
                             "node_path is the node path (e.g. /root/Main/Player), "
                             "method is the method name, args is the method argument array.");
    }
    bool is_meta() const override { return false; }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = String("Node path, e.g. /root/Main/Player");
            p["node_path"] = d;
        }
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = String("Method name");
            p["method"] = d;
        }
        {
            Dictionary d;
            d["type"] = "array";
            d["description"] = String("Method argument array (optional)");
            p["args"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("node_path", "method");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", String::utf8("游戏未运行"));
        }
        Dictionary params;
        params["node_path"] = args_string(ctx.args, "node_path");
        params["method"] = args_string(ctx.args, "method");
        params["args"] = ctx.args.get("args", Array());
        return RuntimeBridge::make_response(bridge->send_command("call_method", params));
    }
};

} // namespace godot_mcp
