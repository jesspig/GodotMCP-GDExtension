// @tool register
#pragma once

#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class RunProjectTool : public ITool {
public:
    String name() const override { return "run_project"; }
    String category() const override { return "editor_tools/runtime"; }
    String brief() const override { return String::utf8("运行项目的默认主场景"); }
    String description() const override {
        return String::utf8("运行项目的默认主场景（在 ProjectSettings 中配置的 application/run/main_scene）。"
                             "等同于在编辑器中按 F5（或点击「运行项目」按钮）。"
                             "场景运行后可通过 stop_project 停止。");
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
        ei->play_main_scene();
        Dictionary data;
        data["action"] = "play_main_scene";
        data["status"] = "started";
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
