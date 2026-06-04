// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class ReparentToNewNodeTool : public ITool {
public:
    String name() const override { return "reparent_to_new_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("用新建的父节点包裹指定节点");
    }
    String description() const override {
        return String::utf8("在原父节点的 index 位置插入一个新节点（new_class 类型），"
                            "然后将 source_node 移入其下。相当于编辑器 \"Reparent to New Node\" 操作。"
                            "新父节点默认继承原父节点的所有者关系。所有变更可撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要包裹的节点路径");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("新父节点的类型（Godot 类名）");
            props["new_class"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("新父节点的名称（留空 = 类型名）");
            props["new_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "new_class");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String new_class = args_string(ctx.args, "new_class");
        String new_name = args_string(ctx.args, "new_name", "");

        if (new_class.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("new_class 不能为空"));
        }
        if (!godot::ClassDB::class_exists(new_class)) {
            return ToolResult::err("UNKNOWN_CLASS",
                String::utf8("未知的 Godot 类: ") + new_class);
        }
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + node_path);
        }
        Node *old_parent = node->get_parent();
        if (!old_parent) {
            return ToolResult::err("ORPHAN_NODE",
                String::utf8("节点无父节点"));
        }
        if (new_name.is_empty()) new_name = new_class;

        Node *wrapper = scene_tree_utils::create_node(new_class, new_name);
        if (!wrapper) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法创建类型为 ") + new_class + String::utf8(" 的节点"));
        }
        if (old_parent->has_node(String("./") + new_name)) {
            memdelete(wrapper);
            return ToolResult::err("NAME_CONFLICT",
                String::utf8("同名节点已存在: ") + new_name);
        }

        int64_t old_index = node->get_index();

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Reparent to New Node"),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);

            // do: add wrapper at source's position, move source under wrapper
            ur->add_do_method(old_parent, "add_child", wrapper, true,
                              (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_do_method(old_parent, "move_child", wrapper, old_index);
            ur->add_do_method(wrapper, "set_owner", ctx.root);
            ur->add_do_method(old_parent, "remove_child", node);
            ur->add_do_method(wrapper, "add_child", node, true,
                              (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_do_reference(wrapper);
            ur->add_do_reference(node);

            // undo: reverse
            ur->add_undo_method(wrapper, "remove_child", node);
            ur->add_undo_method(old_parent, "add_child", node, true,
                                (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_undo_method(old_parent, "move_child", node, old_index);
            ur->add_undo_method(old_parent, "remove_child", wrapper);
            ur->add_undo_reference(wrapper);
            ur->add_undo_reference(node);

            ur->commit_action();
        } else {
            old_parent->add_child(wrapper, true, godot::Node::INTERNAL_MODE_DISABLED);
            old_parent->move_child(wrapper, old_index);
            wrapper->set_owner(ctx.root);
            old_parent->remove_child(node);
            wrapper->add_child(node, true, godot::Node::INTERNAL_MODE_DISABLED);
        }

        Dictionary data;
        data["source"] = relative_path(ctx.root, node);
        data["wrapper"] = relative_path(ctx.root, wrapper);
        data["wrapper_type"] = wrapper->get_class();
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
