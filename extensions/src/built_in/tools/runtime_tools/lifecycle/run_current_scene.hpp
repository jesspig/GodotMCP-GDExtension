// @tool register
#pragma once

#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class RunCurrentSceneTool : public ITool {
public:
    String name() const override { return "run_current_scene"; }
    String category() const override { return "runtime_tools/lifecycle"; }
    String brief() const override { return String::utf8("运行当前在编辑器中打开的场景"); }
    String description() const override {
        return String::utf8("运行当前在编辑器中打开的场景。等同于在编辑器中按 F6（或点击「运行当前场景」按钮）。"
                             "该场景必须包含一个继承自 Node 的根节点。"
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
        String current_scene;
        Node *root = ei->get_edited_scene_root();
        if (root) {
            current_scene = root->get_scene_file_path();
        }
        ei->play_current_scene();
        Dictionary data;
        data["action"] = "play_current_scene";
        data["scene"] = current_scene;
        data["status"] = "started";
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
