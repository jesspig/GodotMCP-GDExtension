// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class RenameNodeTool : public ITool {
public:
    String name() const override { return "rename_node"; }
    String category() const override { return "node"; }
    String brief() const override { return "Rename a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Renames a node in the current scene.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node to rename"; return d; }();
        props["new_name"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "New name for the node"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); req.push_back("new_name"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String nn = args_string(ctx.args, "new_name");
        if (nn.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'new_name'");

        Node *n = ctx.node;
        String old = n->get_name();
        n->set_name(nn);

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Rename " + old + " to " + nn);
            ur->add_do_property(n, "name", nn);
            ur->add_undo_property(n, "name", old);
            ur->commit_action(false);
        }

        Dictionary data;
        data["new_path"] = relative_path(ctx.root, n);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
