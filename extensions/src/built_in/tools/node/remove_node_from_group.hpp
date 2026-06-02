// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class RemoveNodeFromGroupTool : public ITool {
public:
    String name() const override { return "remove_node_from_group"; }
    String category() const override { return "node"; }
    String brief() const override { return "Remove a node from a group"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Removes a node from a named group with undo support.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node"; return d; }();
        props["group"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Group name"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); req.push_back("group"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String group = args_string(ctx.args, "group");
        if (group.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'group'");

        Node *n = ctx.node;
        if (!n->is_in_group(group)) {
            Dictionary data;
            data["node_path"] = relative_path(ctx.root, n);
            data["group"] = group;
            data["hint"] = "Not in group";
            return ToolResult::ok(data);
        }

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Remove " + relative_path(ctx.root, n) + " from group " + group);
            ur->add_do_method(n, "remove_from_group", Variant(group));
            ur->add_undo_method(n, "add_to_group", Variant(group));
            ur->commit_action();
        } else {
            n->remove_from_group(group);
        }

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, n);
        data["group"] = group;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
