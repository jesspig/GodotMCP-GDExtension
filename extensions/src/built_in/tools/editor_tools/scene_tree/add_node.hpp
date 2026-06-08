// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class AddNodeTool : public ITool {
public:
    String name() const override { return "add_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("向场景中添加新节点");
    }
    String description() const override {
        return String::utf8("在指定父节点下创建新节点。class_name 是 Godot 类名（如 Node2D、Node3D、Button）。"
                            "name 留空则使用类型默认名。index=-1 表示追加到末尾。"
                            "所有变更通过 EditorUndoRedoManager 提交，可通过 Ctrl+Z 撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("父节点路径（空 = 场景根）");
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要创建的节点类型（Godot 类名）");
            props["class_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("节点名称（留空 = 类型默认名）");
            props["node_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String::utf8("插入位置（-1 = 追加到末尾）");
            p["default"] = (int64_t)-1;
            props["index"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("class_name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path", "");
        String class_name = args_string(ctx.args, "class_name");
        String node_name = args_string(ctx.args, "node_name");
        int64_t index = args_int(ctx.args, "index", -1);

        if (class_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("class_name 不能为空"));
        }
        if (!godot::ClassDB::class_exists(class_name)) {
            return ToolResult::err("UNKNOWN_CLASS",
                String::utf8("未知的 Godot 类: ") + class_name);
        }
        Node *parent = resolve_node(ctx.root, parent_path);
        if (!parent) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("父节点未找到: ") + parent_path);
        }
        if (node_name.is_empty()) {
            node_name = class_name;
        }

        Node *child = scene_tree_utils::create_node(class_name, node_name);
        if (!child) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法创建类型为 ") + class_name + String::utf8(" 的节点"));
        }

        // 处理 name 冲突
        if (parent->has_node(String("./") + node_name)) {
            memdelete(child);
            return ToolResult::err("NAME_CONFLICT",
                String::utf8("同名节点已存在: ") + node_name);
        }

        EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            parent->add_child(child, true, godot::Node::INTERNAL_MODE_DISABLED);
            child->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            scene_tree_utils::do_add_child(ur, parent, child, ctx.root, index,
                String::utf8("MCP: Add ") + class_name);
        }

        // 选中新节点
        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei) {
            EditorSelection *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(child);
            }
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, child);
        data["type"] = child->get_class();
        data["name"] = child->get_name();
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
