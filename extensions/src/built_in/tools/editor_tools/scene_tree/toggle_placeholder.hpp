#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class TogglePlaceholderTool : public ITool {
public:
    String name() const override { return "toggle_placeholder"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Toggle placeholder loading mode on an instanced scene";
    }
    String description() const override {
        return "enable=true sets the instance as a placeholder (no internal structure expanded, only a placeholder box is shown); "
               "false loads it fully. "
               "When enable is not specified, it automatically toggles the current state. "
               "Only applies to scene instance nodes. All changes are undoable.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Scene instance node path";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "true = enable placeholder, false = disable, empty = auto toggle";
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
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }
        if (node->get_scene_file_path().is_empty()) {
            return ToolResult::err("NOT_AN_INSTANCE",
                "Node is not a scene instance");
        }
        bool current = node->get_scene_instance_load_placeholder();
        bool enable;
        if (ctx.args.has("enable")) {
            enable = static_cast<bool>(ctx.args["enable"]);
        } else {
            enable = !current;
        }
        if (current == enable) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["placeholder"] = enable;
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Toggle Placeholder",
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_scene_instance_load_placeholder", enable);
            ur->add_undo_method(node, "set_scene_instance_load_placeholder", current);
            ur->commit_action();
        } else {
            node->set_scene_instance_load_placeholder(enable);
        }
        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["placeholder"] = enable;
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

