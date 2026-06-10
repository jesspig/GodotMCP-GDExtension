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
    String brief() const override { return String::utf8("调用运行中游戏的节点方法"); }
    String description() const override {
        return String::utf8("在运行中的游戏调用指定节点的任意方法并返回结果。"
                             "参数 node_path 为节点路径（如 /root/Main/Player），"
                             "method 为方法名称，args 为方法参数数组。");
    }
    bool is_meta() const override { return false; }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = String::utf8("节点路径，如 /root/Main/Player");
            p["node_path"] = d;
        }
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = String::utf8("方法名称");
            p["method"] = d;
        }
        {
            Dictionary d;
            d["type"] = "array";
            d["description"] = String::utf8("方法参数数组（可选）");
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
