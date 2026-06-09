// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class SimulateGameInputTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const override { return "simulate_game_input"; }
    String category() const override { return "editor_tools/runtime"; }
    String brief() const override { return String::utf8("向运行中的游戏模拟输入"); }
    String description() const override {
        return String::utf8("向运行中的游戏发送键盘、鼠标或 Action 输入事件。"
                             "actions 为输入动作数组，每个动作包含 type（key/mouse_button/mouse_motion/action）和对应参数。"
                             "示例：\n"
                             "- 按键：{\"type\": \"key\", \"key\": \"W\", \"pressed\": true}\n"
                             "- 鼠标点击：{\"type\": \"mouse_button\", \"button\": \"left\", \"x\": 100, \"y\": 200}\n"
                             "- Action：{\"type\": \"action\", \"action\": \"ui_accept\", \"pressed\": true}");
    }
    bool is_meta() const override { return false; }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary item_schema;
            item_schema["type"] = "object";
            Dictionary item_props;
            {
                Dictionary tp;
                tp["type"] = "string";
                tp["description"] = "动作类型：key / mouse_button / mouse_motion / action";
                item_props["type"] = tp;
            }
            {
                Dictionary kp;
                kp["type"] = "string";
                kp["description"] = "键名（key 类型时必填），如 W、Space、Escape";
                item_props["key"] = kp;
            }
            {
                Dictionary bp;
                bp["type"] = "string";
                bp["description"] = "鼠标按钮（mouse_button 类型时）：left / right / middle";
                item_props["button"] = bp;
            }
            {
                Dictionary ap;
                ap["type"] = "string";
                ap["description"] = "Action 名称（action 类型时必填）";
                item_props["action"] = ap;
            }
            item_schema["properties"] = item_props;
            item_schema["required"] = Array::make("type");

            Dictionary d;
            d["type"] = "array";
            d["description"] = String::utf8("输入动作数组");
            d["items"] = item_schema;
            p["actions"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("actions");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", String::utf8("游戏未运行"));
        }
        Dictionary params;
        params["actions"] = ctx.args.get("actions", Array());
        return bridge->send_command("simulate_input", params);
    }
};

} // namespace godot_mcp
