#pragma once

#include "toggle_base.hpp"

namespace godot_mcp {

class ToggleEditGroupTool : public ToggleBase {
public:
    String name() const noexcept override { return "toggle_edit_group"; }
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
