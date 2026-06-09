// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class GetGameNodePropertyTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const override { return "get_game_node_property"; }
    String category() const override { return "runtime_tools/bridge"; }
    String brief() const override { return String::utf8("读取运行中游戏的节点属性"); }
    String description() const override {
        return String::utf8("读取运行中游戏的指定节点的属性值。"
                             "参数 node_path 为节点路径（如 /root/Main/Player），"
                             "property 为属性名称（如 position、rotation 等）。");
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
            d["description"] = String::utf8("属性名称，如 position");
            p["property"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("node_path", "property");
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
        params["property"] = args_string(ctx.args, "property");
        return bridge->send_command("get_property", params);
    }
};

} // namespace godot_mcp
