// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

namespace godot_mcp {

class BranchToSceneTool : public ITool {
public:
    String name() const override { return "branch_to_scene"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Extract a node into a new scene file"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Extracts a node (and its children) into a separate .tscn file, then replaces it "
               "with an instance of that new scene. The scene root itself cannot be branched.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node to branch"; return d; }();
        props["scene_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Destination path for the new scene file"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); req.push_back("scene_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String p = args_string(ctx.args, "node_path");
        const String sp = args_string(ctx.args, "scene_path");
        if (sp.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'scene_path'");

        Node *n = ctx.node;
        Node *root = ctx.root;

        if (n == root) return ToolResult::err("CANNOT_BRANCH_ROOT", "Cannot branch scene root");
        Node *parent = n->get_parent();
        if (!parent) return ToolResult::err("NO_PARENT", "Node has no parent");

        Node *saved = Object::cast_to<Node>(n->duplicate());
        if (!saved) return ToolResult::err("DUPLICATE_FAILED", "Failed to duplicate node");

        Ref<PackedScene> ps;
        ps.instantiate();
        if (ps->pack(saved) != OK) return ToolResult::err("PACK_FAILED", "Failed to pack scene");

        ensure_parent_dir(sp);
        if (ResourceSaver::get_singleton()->save(ps, sp) != OK) {
            return ToolResult::err("SAVE_FAILED", "Failed to save scene");
        }
        notify_file_changed(sp);

        Ref<PackedScene> loaded = ResourceLoader::get_singleton()->load(sp);
        if (loaded.is_null()) return ToolResult::err("RELOAD_FAILED", "Failed to reload saved scene");

        Node *inst = Object::cast_to<Node>(loaded->instantiate());
        if (!inst) return ToolResult::err("INSTANTIATE_FAILED", "Failed to instantiate saved scene");
        inst->set_name(n->get_name());

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Branch to Scene: " + sp);
            ur->add_undo_method(parent, "add_child", Variant(saved));
            ur->add_undo_method(saved, "set_owner", Variant(root));
            ur->add_undo_reference(saved);
            ur->commit_action(false);
        }

        parent->remove_child(n);
        n->queue_free();
        parent->add_child(inst);
        inst->set_owner(root);

        Dictionary data;
        data["scene_path"] = sp;
        data["node_path"] = p;
        data["replaced_with_instance"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
