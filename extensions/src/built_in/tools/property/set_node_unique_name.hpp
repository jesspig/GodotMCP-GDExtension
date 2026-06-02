// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class SetNodeUniqueNameTool : public ITool {
public:
    String name() const override { return "set_node_unique_name"; }
    String category() const override { return "property/2d"; }
    String brief() const override { return "Set or unset a node's unique name in owner"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Toggles the %unique_name_in_owner flag on a node with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["unique"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Unique name state";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        bool unique = args_bool(ctx.args, "unique", true);
        Node *n = ctx.node;
        bool old = n->is_unique_name_in_owner();
        EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) { n->set("unique_name_in_owner", unique); return ToolResult::ok(Dictionary()); }
        ur->create_action("Set unique_name for " + relative_path(ctx.root, n));
        ur->add_do_method(n, "set_unique_name_in_owner", Variant(unique));
        ur->add_undo_method(n, "set_unique_name_in_owner", Variant(old));
        ur->commit_action(false);
        Dictionary d; d["node_path"] = relative_path(ctx.root, n); d["unique_name_in_owner"] = unique;
        return ToolResult::ok(d);
    }
};

} // namespace
