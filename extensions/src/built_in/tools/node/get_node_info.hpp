// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

class GetNodeInfoTool : public ITool {
public:
    String name() const override { return "get_node_info"; }
    String category() const override { return "node"; }
    String brief() const override { return "Get detailed info about a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Returns comprehensive metadata about a node: name, type, path, script, owner, groups, etc.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Absolute or relative node path"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Node *n = ctx.node;
        Node *root = ctx.root;

        Ref<Script> sc = n->get("script");
        Array grps = n->get_groups();
        Array garr;
        for (int i = 0; i < grps.size(); i++) garr.append(grps[i]);

        Dictionary data;
        data["name"] = n->get_name();
        data["type"] = n->get_class();
        data["path"] = relative_path(root, n);
        data["script"] = sc.is_valid() ? Variant(sc->get_path()) : Variant();
        data["visible"] = (bool)n->get("visible");
        data["groups"] = garr;
        data["child_count"] = n->get_child_count();
        data["unique_name"] = n->is_unique_name_in_owner();

        String sf = n->get_scene_file_path();
        data["is_instance"] = !sf.is_empty();
        data["scene_file_path"] = sf.is_empty() ? Variant() : Variant(sf);

        Node *owner = n->get_owner();
        data["owner"] = owner ? Variant(relative_path(root, owner)) : Variant();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
