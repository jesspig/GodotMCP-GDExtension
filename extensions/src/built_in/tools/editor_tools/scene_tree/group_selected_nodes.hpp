// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class GroupSelectedNodesTool : public ITool {
public:
    String name() const override { return "group_selected_nodes"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("将编辑器中选中的多个节点编组到新建父节点下");
    }
    String description() const override {
        return String::utf8("等效于编辑器 \"Group Selected\" 操作。"
                            "读取 EditorSelection 中的所有顶层节点，在它们的公共父节点下创建一个新节点（new_class 类型），"
                            "然后把所有选中节点移入新节点。"
                            "要求选中节点至少 2 个且共享同一父节点。"
                            "所有变更可撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("新父节点的类型（Godot 类名）");
            p["default"] = "Node";
            props["new_class"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("新父节点名（留空 = 类型名）");
            props["new_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String new_class = args_string(ctx.args, "new_class", "Node");
        String new_name = args_string(ctx.args, "new_name", "");
        if (!godot::ClassDB::class_exists(new_class)) {
            return ToolResult::err("UNKNOWN_CLASS",
                String::utf8("未知的 Godot 类: ") + new_class);
        }
        if (new_name.is_empty()) new_name = new_class;

        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", String::utf8("EditorInterface 不可用"));
        }
        EditorSelection *sel = ei->get_selection();
        if (!sel) {
            return ToolResult::err("NO_SELECTION", String::utf8("无法获取 EditorSelection"));
        }
        godot::TypedArray<godot::Node> top = sel->get_top_selected_nodes();
        if (top.size() < 2) {
            return ToolResult::err("INSUFFICIENT_SELECTION",
                String::utf8("需要至少 2 个选中顶层节点（当前: ") +
                String::num_int64((int64_t)top.size()) + String(")"));
        }
        // Validate all share the same parent
        Node *common_parent = nullptr;
        godot::TypedArray<int64_t> indices;
        for (int i = 0; i < top.size(); i++) {
            Node *n = godot::Object::cast_to<godot::Node>(top[i]);
            if (!n) continue;
            Node *p = n->get_parent();
            if (!p) {
                return ToolResult::err("ORPHAN_SELECTED",
                    String::utf8("选中的节点 ") + n->get_name() +
                    String::utf8(" 无父节点"));
            }
            if (common_parent == nullptr) {
                common_parent = p;
            } else if (p != common_parent) {
                return ToolResult::err("DIFFERENT_PARENTS",
                    String::utf8("选中的节点必须共享同一父节点"));
            }
        }
        if (!common_parent) {
            return ToolResult::err("NO_VALID_SELECTION",
                String::utf8("选中的节点无效"));
        }
        if (common_parent->has_node(String("./") + new_name)) {
            return ToolResult::err("NAME_CONFLICT",
                String::utf8("同名节点已存在: ") + new_name);
        }

        Node *wrapper = scene_tree_utils::create_node(new_class, new_name);
        if (!wrapper) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法创建类型为 ") + new_class + String::utf8(" 的节点"));
        }
        scene_tree_utils::assign_owner_recursive(wrapper, ctx.root);

        // capture indices and nodes for undo
        godot::Array nodes_arr;
        godot::Array indices_arr;
        for (int i = 0; i < top.size(); i++) {
            Node *n = godot::Object::cast_to<godot::Node>(top[i]);
            if (!n) continue;
            nodes_arr.append(n);
            indices_arr.append((int64_t)n->get_index());
        }

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Group Selected Nodes"),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);

            // do: add wrapper at the smallest index among selected
            int64_t min_idx = (int64_t)indices_arr[0];
            for (int i = 1; i < indices_arr.size(); i++) {
                if ((int64_t)indices_arr[i] < min_idx) min_idx = (int64_t)indices_arr[i];
            }
            ur->add_do_method(common_parent, "add_child", wrapper, true,
                              (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_do_method(common_parent, "move_child", wrapper, min_idx);

            // do: move each selected into wrapper
            for (int i = 0; i < nodes_arr.size(); i++) {
                Node *n = godot::Object::cast_to<godot::Node>(nodes_arr[i]);
                int64_t old_idx = (int64_t)indices_arr[i];
                ur->add_do_method(common_parent, "remove_child", n);
                ur->add_do_method(wrapper, "add_child", n, true,
                                  (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
                ur->add_do_reference(n);
                ur->add_undo_reference(n);
                // undo: move back
                ur->add_undo_method(wrapper, "remove_child", n);
                ur->add_undo_method(common_parent, "add_child", n, true,
                                    (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
                ur->add_undo_method(common_parent, "move_child", n, old_idx);
            }

            ur->add_do_reference(wrapper);
            ur->add_undo_reference(wrapper);
            ur->commit_action();
        } else {
            int64_t min_idx = (int64_t)indices_arr[0];
            for (int i = 1; i < indices_arr.size(); i++) {
                if ((int64_t)indices_arr[i] < min_idx) min_idx = (int64_t)indices_arr[i];
            }
            common_parent->add_child(wrapper, true, godot::Node::INTERNAL_MODE_DISABLED);
            common_parent->move_child(wrapper, min_idx);
            for (int i = 0; i < nodes_arr.size(); i++) {
                Node *n = godot::Object::cast_to<godot::Node>(nodes_arr[i]);
                common_parent->remove_child(n);
                wrapper->add_child(n, true, godot::Node::INTERNAL_MODE_DISABLED);
            }
        }

        Dictionary data;
        data["wrapper"] = relative_path(ctx.root, wrapper);
        data["wrapper_type"] = wrapper->get_class();
        data["grouped"] = (int64_t)nodes_arr.size();
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
