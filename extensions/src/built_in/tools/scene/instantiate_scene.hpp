// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class InstantiateSceneTool : public ITool {
public:
    String name() const override { return "instantiate_scene"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Instantiate a scene file under a parent node"; }
    bool needs_scene() const override { return true; }
    String description() const override {
        return "Loads a .tscn file and adds it as a child of the specified parent node in the current scene.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["scene_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the scene file to instantiate"; return d; }();
        props["parent_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Parent node path to attach under"; return d; }();
        props["name"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Optional name for the new instance"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("scene_path"); req.push_back("parent_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String sp = args_string(ctx.args, "scene_path");
        const String pp = args_string(ctx.args, "parent_path");
        Node *root = ctx.root;

        Node *parent = resolve_node(root, pp);
        if (!parent) return ToolResult::err("PARENT_NOT_FOUND", "Parent not found: " + pp);

        Ref<PackedScene> packed = ResourceLoader::get_singleton()->load(sp);
        if (packed.is_null()) return ToolResult::err("LOAD_FAILED", "Scene '" + sp + "' could not be loaded");

        Node *inst = Object::cast_to<Node>(packed->instantiate());
        if (!inst) return ToolResult::err("INSTANTIATE_FAILED", "Failed to instantiate scene: " + sp);

        String name = args_string(ctx.args, "name");
        if (!name.is_empty()) inst->set_name(name);

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Instantiate Scene: " + sp);
            ur->add_do_method(parent, "add_child", Variant(inst));
            ur->add_do_method(inst, "set_owner", Variant(root));
            ur->add_do_reference(inst);
            ur->add_undo_method(parent, "remove_child", Variant(inst));
            ur->commit_action();
        }

        Dictionary data;
        data["path"] = relative_path(root, inst);
        data["parent"] = relative_path(root, parent);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
