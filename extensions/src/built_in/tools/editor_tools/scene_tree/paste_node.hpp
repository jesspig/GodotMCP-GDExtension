// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class PasteNodeTool : public ITool {
public:
    String name() const override { return "paste_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("从剪贴板粘贴节点");
    }
    String description() const override {
        return String::utf8("从内部 PackedScene 剪贴板实例化节点。mode: child（作为 target_path 的子节点，默认）、"
                            "sibling（作为 target_path 的同级节点，添加在它之后）、"
                            "replacement（替换 target_path 节点）。"
                            "剪贴板为空时返回错误。Ctrl+Z 可恢复被替换节点。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标节点路径（mode=sibling/replacement 时必填）");
            props["target_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("child | sibling | replacement");
            p["default"] = "child";
            p["enum"] = Array::make("child", "sibling", "replacement");
            props["mode"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("新节点名（留空 = 保持剪贴板中的名字）");
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
        String target_path = args_string(ctx.args, "target_path", "");
        String mode = args_string(ctx.args, "mode", "child");
        String new_name = args_string(ctx.args, "new_name", "");

        Ref<PackedScene> clipboard = scene_tree_utils::get_clipboard();
        if (clipboard.is_null()) {
            return ToolResult::err("EMPTY_CLIPBOARD",
                String::utf8("剪贴板为空，请先用 copy_node 或 cut_node 复制节点"));
        }

        Node *target = nullptr;
        Node *parent = nullptr;
        int64_t target_index = -1;
        if (!target_path.is_empty()) {
            target = resolve_node(ctx.root, target_path);
            if (!target) {
                return ToolResult::err("TARGET_NOT_FOUND",
                    String::utf8("目标节点未找到: ") + target_path);
            }
        }
        if (mode == "child") {
            parent = target ? target : ctx.root;
        } else if (mode == "sibling") {
            if (!target) {
                return ToolResult::err("MISSING_TARGET",
                    String::utf8("sibling 模式需要 target_path"));
            }
            parent = target->get_parent();
            if (!parent) {
                return ToolResult::err("ORPHAN_TARGET",
                    String::utf8("目标节点无父节点"));
            }
            target_index = target->get_index() + 1;
        } else if (mode == "replacement") {
            if (!target) {
                return ToolResult::err("MISSING_TARGET",
                    String::utf8("replacement 模式需要 target_path"));
            }
            parent = target->get_parent();
            if (!parent) {
                return ToolResult::err("ORPHAN_TARGET",
                    String::utf8("目标节点无父节点"));
            }
            target_index = target->get_index();
        } else {
            return ToolResult::err("BAD_MODE",
                String::utf8("mode 必须是 child | sibling | replacement"));
        }

        Node *inst = scene_tree_utils::instantiate_subtree(clipboard);
        if (!inst) {
            return ToolResult::err("INSTANTIATE_FAILED",
                String::utf8("无法从剪贴板实例化节点"));
        }
        if (!new_name.is_empty()) {
            inst->set_name(new_name);
        }
        // owner wired post-add_child (use a do_method so it captures in undo)

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Paste ") + inst->get_name(),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            if (mode == "replacement") {
                ur->add_do_method(target, "replace_by", inst, true);
                ur->add_undo_method(inst, "replace_by", target, true);
                ur->add_do_reference(inst);
                ur->add_undo_reference(inst);
                ur->add_do_reference(target);
                ur->add_undo_reference(target);
            } else {
                ur->add_do_method(parent, "add_child", inst, true,
                                  (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
                if (target_index >= 0) {
                    ur->add_do_method(parent, "move_child", inst, target_index);
                }
                ur->add_undo_method(parent, "remove_child", inst);
                ur->add_do_reference(inst);
                ur->add_undo_reference(inst);
            }
            ur->commit_action();

            // After commit, set_owner
            scene_tree_utils::assign_owner_recursive(inst, ctx.root);

            // In replacement mode, also re-set owner on target subtree
            if (mode == "replacement") {
                // (inst has now replaced target; ensure all its descendants have owner)
                scene_tree_utils::assign_owner_recursive(inst, ctx.root);
            }
        } else {
            if (mode == "replacement") {
                target->replace_by(inst, true);
            } else {
                parent->add_child(inst, true, godot::Node::INTERNAL_MODE_DISABLED);
                if (target_index >= 0) {
                    parent->move_child(inst, target_index);
                }
            }
            scene_tree_utils::assign_owner_recursive(inst, ctx.root);
        }

        // select new node
        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei) {
            EditorSelection *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(inst);
            }
        }

        Dictionary data;
        data["new_node"] = relative_path(ctx.root, inst);
        data["type"] = inst->get_class();
        data["mode"] = mode;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
