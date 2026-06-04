// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class NewSceneTool : public ITool {
public:
    String name() const override { return "new_scene"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("创建新场景，指定根节点类型和名称");
    }
    String description() const override {
        return String::utf8("在编辑器中新建一个空白场景，根节点类型由 class_name 指定（如 \"Node2D\"、\"Node3D\"、\"Control\"、\"Node\"）。"
                            "可指定根节点名称；场景保存在内存中，可用 save_scene 写入 res:// 路径。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("根节点类型（Godot 类名，如 Node2D、Node3D、Control、Node）");
            props["root_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("根节点名称（默认 = 类型名）");
            props["root_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("root_type");
        return s;
    }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String root_type = args_string(ctx.args, "root_type");
        String root_name = args_string(ctx.args, "root_name");
        if (root_type.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("root_type 不能为空"));
        }
        if (!godot::ClassDB::class_exists(root_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                String::utf8("未知的 Godot 类: ") + root_type);
        }
        if (root_name.is_empty()) {
            root_name = root_type;
        }

        Node *new_root = scene_tree_utils::create_node(root_type, root_name);
        if (!new_root) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法创建类型为 ") + root_type + String::utf8(" 的节点"));
        }

        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            memdelete(new_root);
            return ToolResult::err("NO_EDITOR", String::utf8("EditorInterface 不可用"));
        }
        ei->add_root_node(new_root);

        EditorSelection *sel = ei->get_selection();
        if (sel) {
            sel->clear();
            sel->add_node(new_root);
        }

        Dictionary data;
        data["root_type"] = root_type;
        data["root_name"] = new_root->get_name();
        data["path"] = ".";
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
