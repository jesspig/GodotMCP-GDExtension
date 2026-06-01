// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

namespace godot_mcp {

class CreateSceneTool : public ITool {
public:
    String name() const override { return "create_scene"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Create a new .tscn scene file"; }
    String description() const override {
        return "Creates a new scene file at the specified path with the given root node type and name.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Scene file path (must end with .tscn)"; return d; }();
        props["root_type"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Root node class name (default: Node)"; return d; }();
        props["root_name"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Root node name (default: Root)"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String p = args_string(ctx.args, "path");
        if (p.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'path'");
        if (!p.ends_with(".tscn")) return ToolResult::err("INVALID_PATH", "path must end with .tscn");
        if (FileAccess::file_exists(p)) return ToolResult::err("ALREADY_EXISTS", "File already exists: " + p);

        const String rt = args_string(ctx.args, "root_type", "Node");
        const String rn = args_string(ctx.args, "root_name", "Root");
        ensure_parent_dir(p);

        Node *root = Object::cast_to<Node>(ClassDB::instantiate(rt));
        if (!root) return ToolResult::err("BAD_ROOT_TYPE", "Cannot instantiate root type: " + rt);
        root->set_name(rn);

        Ref<PackedScene> ps;
        ps.instantiate();
        ps->pack(root);
        Error err = ResourceSaver::get_singleton()->save(ps, p);
        if (err != OK) return ToolResult::err("SAVE_FAILED", "Failed to save scene: " + p);

        notify_file_changed(p);

        Dictionary data;
        data["path"] = p;
        data["created"] = true;
        data["root_type"] = rt;
        data["root_name"] = rn;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
