// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class ExecuteInGameTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const override { return "execute_in_game"; }
    String category() const override { return "editor_tools/runtime"; }
    String brief() const override { return String::utf8("在运行中的游戏执行 GDScript"); }
    String description() const override {
        return String::utf8("在运行中的游戏进程内执行任意 GDScript 代码并返回结果。"
                             "脚本必须定义 func execute(scene_tree: SceneTree) -> Variant 方法。"
                             "示例：\nextends RefCounted\n"
                             "func execute(scene_tree):\n"
                             "    var player = scene_tree.root.get_node(\"Player\")\n"
                             "    return player.position");
    }
    bool is_meta() const override { return false; }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = String::utf8("GDScript 源码，必须包含 func execute(scene_tree) 方法");
            p["source"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("source");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", String::utf8("游戏未运行"));
        }
        Dictionary params;
        params["source"] = args_string(ctx.args, "source");
        return bridge->send_command("execute_script", params);
    }
};

} // namespace godot_mcp
