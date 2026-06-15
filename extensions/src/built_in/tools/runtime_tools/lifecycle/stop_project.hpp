
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
    String brief() const override { return String("Stop the running project"); }
    String description() const override {
        return String("Stops the currently running project. Equivalent to pressing F8 (or clicking the Stop button) in the editor. "
                             "Does nothing if the project is not running.");
    }
    Dictionary build_input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
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

