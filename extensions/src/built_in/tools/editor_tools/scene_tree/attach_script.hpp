// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

namespace godot_mcp {

class AttachScriptTool : public ITool {
public:
    String name() const override { return "attach_script"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Attach a GDScript/C# script to a node";
    }
    String description() const override {
        return "Loads a script from a res:// path and binds it to a node via set_script. "
               "Automatically notifies EditorFileSystem to refresh so the new script takes effect. "
               "If the node already has a script, it will be replaced by the new one (Ctrl+Z restores the old script). "
               "If the script type does not match the node (e.g. a script inheriting from Node3D but the node is Node2D), "
               "the attach will still be allowed but may produce errors at runtime.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Script res:// path (.gd, .cs, etc.)";
            props["script_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "script_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String script_path = args_string(ctx.args, "script_path");
        if (script_path.is_empty()) {
            return ToolResult::err("MISSING_ARG", "script_path cannot be empty");
        }
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }

        godot::Ref<godot::Resource> res =
            godot::ResourceLoader::get_singleton()->load(script_path);
        if (res.is_null()) {
            return ToolResult::err("LOAD_FAILED",
                "Failed to load script: " + script_path);
        }
        godot::Ref<godot::Script> script = res;
        if (script.is_null()) {
            return ToolResult::err("NOT_A_SCRIPT",
                "Resource is not a Script type: " + script_path);
        }

        godot::Ref<godot::Script> old_script = node->get_script();

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Attach Script " + script_path,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_script", script);
            if (old_script.is_valid()) {
                ur->add_undo_method(node, "set_script", old_script);
            } else {
                ur->add_undo_method(node, "set_script", godot::Variant());
            }
            ur->commit_action();
        } else {
            node->set_script(script);
        }

        notify_file_changed(script_path);

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["script"] = script_path;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
