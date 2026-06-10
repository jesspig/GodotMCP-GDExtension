// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class StopProjectTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }
    String name() const override { return "stop_project"; }
    String category() const override { return "runtime_tools/lifecycle"; }
    String brief() const override { return String::utf8("停止运行中的项目"); }
    String description() const override {
        return String::utf8("停止当前正在运行的项目。等同于在编辑器中按 F8（或点击「停止」按钮）。"
                             "如果项目未运行则不做任何操作。");
    }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", String::utf8("EditorInterface 不可用"));
        }
        bool was_playing = ei->is_playing_scene();
        ei->stop_playing_scene();

        // Disconnect bridge immediately so tools don't try to use stale connection
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (bridge) {
            bridge->disconnect();
        }

        Dictionary data;
        data["action"] = "stop_playing_scene";
        data["was_running"] = was_playing;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
