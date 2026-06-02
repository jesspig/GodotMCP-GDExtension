// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class SceneToBranchTool : public ITool {
public:
    String name() const override { return "scene_to_branch"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Convert an instanced scene node back to a regular node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Detaches a node from its instanced scene file, converting it into a regular editable node. "
               "The opposite of branch_to_scene.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the instanced scene node"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String p = args_string(ctx.args, "node_path");
        Node *n = ctx.node;
        String sf = n->get_scene_file_path();

        if (sf.is_empty()) {
            return ToolResult::err("NOT_INSTANCED", "Node is not an instanced scene; use branch_to_scene");
        }

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Scene to Branch: " + p);
            ur->add_do_method(n, "set_scene_file_path", Variant(""));
            ur->add_undo_method(n, "set_scene_file_path", Variant(sf));
            ur->commit_action();
        }
        n->set_scene_file_path("");

        Dictionary data;
        data["node_path"] = p;
        data["converted"] = true;
        data["source_scene"] = sf;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
