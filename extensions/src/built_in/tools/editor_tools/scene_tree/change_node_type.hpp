// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class ChangeNodeTypeTool : public ITool {
public:
    String name() const override { return "change_node_type"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("修改节点的类型（保持名字、子节点、属性映射）");
    }
    String description() const override {
        return String::utf8("使用 Node::replace_by 交换节点类型。新类型必须继承自原类型（is_parent_class 检查）。"
                            "name 留空 = 保留原名。可用 property_mapping 调整属性值（如新类型的属性名不同）。"
                            "注意：原类型特有的属性值会丢失。所有变更可撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要修改类型的节点路径");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("新类型（Godot 类名）");
            props["new_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("新节点名（留空 = 保留原名）");
            props["new_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = String::utf8("属性映射 {old_property: new_property}（可选）");
            props["property_mapping"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "new_type");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String new_type = args_string(ctx.args, "new_type");
        String new_name = args_string(ctx.args, "new_name", "");
        Variant prop_mapping_var = ctx.args.get("property_mapping", Variant());

        if (new_type.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("new_type 不能为空"));
        }
        if (!godot::ClassDB::class_exists(new_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                String::utf8("未知的 Godot 类: ") + new_type);
        }
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + node_path);
        }
        String old_type = node->get_class();
        if (old_type == new_type) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        // check inheritance: new_type must be a subclass of old_type
        // ClassDB::is_parent_class expects (class, parent). It returns true if `class`
        // inherits from `parent`.
        if (!godot::ClassDB::is_parent_class(new_type, old_type) &&
            old_type != new_type) {
            return ToolResult::err("TYPE_INCOMPATIBLE",
                String::utf8("新类型 ") + new_type +
                String::utf8(" 必须继承自 ") + old_type);
        }

        Node *parent = node->get_parent();
        if (!parent) {
            return ToolResult::err("ORPHAN_NODE",
                String::utf8("节点无父节点"));
        }
        int64_t old_index = node->get_index();

        // Create new instance
        Node *new_node = scene_tree_utils::create_node(new_type, "");
        if (!new_node) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法创建类型为 ") + new_type + String::utf8(" 的节点"));
        }
        if (!new_name.is_empty()) {
            new_node->set_name(new_name);
        } else {
            new_node->set_name(node->get_name());
        }

        // apply property mapping if provided
        if (prop_mapping_var.get_type() == godot::Variant::DICTIONARY) {
            godot::Dictionary mapping = prop_mapping_var;
            godot::Array keys = mapping.keys();
            for (int i = 0; i < keys.size(); i++) {
                String old_key = keys[i];
                String new_key = mapping[old_key];
                if (node->has_meta(old_key)) {
                    Variant v = node->get_meta(old_key);
                    new_node->set_meta(new_key, v);
                } else {
                    Variant v = node->get(old_key);
                    if (v.get_type() != godot::Variant::NIL) {
                        new_node->set(new_key, v);
                    }
                }
            }
        }

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Change Type ") + old_type +
                                  String::utf8(" → ") + new_type,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "replace_by", new_node, true);
            ur->add_undo_method(new_node, "replace_by", node, true);
            ur->add_do_reference(new_node);
            ur->add_undo_reference(new_node);
            ur->add_do_reference(node);
            ur->add_undo_reference(node);
            ur->commit_action();

            // After replace_by, new_node takes the place of node.
            // re-set the index (replace_by may or may not preserve order).
            if (parent->get_child_count() > 0) {
                // Find new_node in parent
                int64_t idx = -1;
                for (int64_t i = 0; i < parent->get_child_count(); i++) {
                    if (parent->get_child(i) == new_node) { idx = i; break; }
                }
                if (idx >= 0 && idx != old_index) {
                    parent->move_child(new_node, old_index);
                }
            }
        } else {
            node->replace_by(new_node, true);
            if (parent->get_child_count() > 0) {
                int64_t idx = -1;
                for (int64_t i = 0; i < parent->get_child_count(); i++) {
                    if (parent->get_child(i) == new_node) { idx = i; break; }
                }
                if (idx >= 0 && idx != old_index) {
                    parent->move_child(new_node, old_index);
                }
            }
        }

        Dictionary data;
        data["old_type"] = old_type;
        data["new_type"] = new_type;
        data["new_name"] = new_node->get_name();
        data["new_path"] = relative_path(ctx.root, new_node);
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
