
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class ToggleEditGroupTool : public ITool {
public:
    String name() const noexcept override { return "toggle_edit_group"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Toggle a node's edit group state (_edit_group_ meta)";
    }
    String description() const override {
        return "enable=true sets the _edit_group_ meta on the node (group edit mode, children can be independently edited in the parent scene context); "
               "false removes the meta. "
               "When enable is not specified, it automatically toggles the current state. "
               "This is distinct from toggle_lock (_edit_lock_) metadata. "
               "All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "true = enable group edit, false = disable, empty = auto toggle";
            props["enable"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        bool current = node->has_meta("_edit_group_");
        bool enable;
        if (ctx.args.has("enable")) {
            enable = static_cast<bool>(ctx.args["enable"]);
        } else {
            enable = !current;
        }
        if (current == enable) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["edit_group"] = enable;
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        auto *ur = begin_undo_action("MCP: Toggle Edit Group");
        if (ur) {
            if (enable) {
                ur->add_do_method(node, "set_meta", "_edit_group_",
                                  godot::Variant());
                ur->add_undo_method(node, "remove_meta", "_edit_group_");
            } else {
                ur->add_do_method(node, "remove_meta", "_edit_group_");
                ur->add_undo_method(node, "set_meta", "_edit_group_",
                                    godot::Variant());
            }
            commit_undo_action(ur);
        } else {
            if (enable) {
                node->set_meta("_edit_group_", godot::Variant());
            } else {
                node->remove_meta("_edit_group_");
            }
        }
        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["edit_group"] = enable;
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
