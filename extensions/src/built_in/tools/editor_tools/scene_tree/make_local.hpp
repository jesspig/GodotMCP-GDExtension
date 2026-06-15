
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot_mcp {

class MakeLocalTool : public ITool {
public:
    String name() const noexcept override { return "make_local"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Make a scene instance local (break external reference)";
    }
    String description() const override {
        return "Clears the node's scene_file_path, converting the node and its subtree to local "
               "(no longer linked to the original .tscn). "
               "All ownership is auto-rewritten to the current scene root. "
               "Equivalent to the editor's Make Local operation. Only applies to scene instance nodes. "
               "All changes are undoable (restoring scene_file_path has limited support �?old value must be recorded first).";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Scene instance node path";
            props["node_path"] = p;
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
        String old_sfp = node->get_scene_file_path();
        if (old_sfp.is_empty()) {
            return ToolResult::err("NOT_AN_INSTANCE",
                "Node is not a scene instance (no scene_file_path)");
        }

        auto *ur = begin_undo_action("MCP: Make Local " + node->get_name());
        if (ur) {
            ur->add_do_method(node, "set_scene_file_path", String());
            ur->add_undo_method(node, "set_scene_file_path", old_sfp);
            commit_undo_action(ur);
        } else {
            node->set_scene_file_path("");
        }

        // Re-assign ownership recursively (this is the second part of Make Local)
        scene_tree_utils::assign_owner_recursive(node, ctx.root);

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["old_scene_file_path"] = old_sfp;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
