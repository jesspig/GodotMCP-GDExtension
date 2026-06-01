// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class DetachScriptTool : public ITool {
public:
    String name() const override { return "detach_script"; }
    String category() const override { return "node"; }
    String brief() const override { return "Detach the script from a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Removes any attached script from a node with undo support.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the target node"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Node *n = ctx.node;
        Variant old = n->get("script");

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Detach Script from " + relative_path(ctx.root, n));
            ur->add_do_property(n, "script", Variant());
            ur->add_undo_property(n, "script", old);
            ur->commit_action();
        } else {
            n->set("script", Variant());
        }

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, n);
        data["script"] = Variant();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
