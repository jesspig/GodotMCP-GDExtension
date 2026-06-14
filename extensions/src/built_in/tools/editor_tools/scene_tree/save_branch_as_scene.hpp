#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/object.hpp>

namespace godot_mcp {

class SaveBranchAsSceneTool : public ITool {
public:
    String name() const override { return "save_branch_as_scene"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Save a node branch as a standalone .tscn scene";
    }
    String description() const override {
        return "Packs the specified node and its subtree into a PackedScene and writes it to a res:// path. "
               "After saving, the branch can be re-instantiated using the instance_child_scene tool. "
               "This tool performs a structural transformation �?undo does not restore the file (only affects the node structure).";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path (empty = root node)";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target res:// path (must end with .tscn)";
            props["path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        String path = args_string(ctx.args, "path");
        if (path.is_empty()) {
            return ToolResult::err("MISSING_ARG", "path cannot be empty");
        }
        if (!path.ends_with(".tscn")) {
            return ToolResult::err("BAD_EXTENSION",
                "Path must end with .tscn");
        }

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }
        if (node == ctx.root) {
            return ToolResult::err("ROOT_NOT_ALLOWED",
                "Cannot save the scene root as a sub-scene, please select another node");
        }

        if (!ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                "Failed to create parent directory: " + path);
        }

        // Temporarily clear the node's scene_file_path to prevent .tscn nesting pointing to itself
        String old_sfp = node->get_scene_file_path();
        node->set_scene_file_path("");

        godot::Ref<godot::PackedScene> packed = scene_tree_utils::pack_subtree(node);
        if (packed.is_null()) {
            node->set_scene_file_path(old_sfp);
            return ToolResult::err("PACK_FAILED",
                "Failed to pack node");
        }

        godot::Error err = godot::ResourceSaver::get_singleton()->save(packed, path);
        node->set_scene_file_path(old_sfp);
        if (err != godot::OK) {
            return ToolResult::err("SAVE_FAILED",
                "Save failed, error code: " + String::num_int64(static_cast<int64_t>(err)));
        }
        notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["node"] = relative_path(ctx.root, node);
        data["child_count"] = static_cast<int64_t>(node->get_child_count());
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

