#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource.hpp>

namespace godot_mcp {

class GetResourceInfoTool : public ITool {
public:
    String name() const noexcept override { return "get_resource_info"; }
    String category() const noexcept override { return "node_tools/general"; }
    String brief() const noexcept override {
        return String("Get resource property metadata");
    }
    String description() const override {
        return String("Returns metadata of the resource on a specified node property, including type, path, name, whether it is built-in, and a list of all readable/writable properties.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Node path (empty = root node of current edited scene)");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Property name (e.g. texture, material)");
            props["property_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("property_name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String prop_name = args_string(ctx.args, "property_name");

        if (prop_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("property_name cannot be empty"));
        }

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        Variant val = node->get(prop_name);
        Object *obj = val;
        if (!obj) {
            return ToolResult::err("NOT_A_RESOURCE",
                String("Property does not currently have a value"));
        }

        auto *res = Object::cast_to<godot::Resource>(obj);
        if (!res) {
            return ToolResult::err("NOT_A_RESOURCE",
                String("Property is not a Resource type: ") + obj->get_class());
        }

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        data["resource_type"] = res->get_class();
        data["resource_path"] = res->get_path();
        data["resource_name"] = res->get_name();
        data["is_built_in"] = res->is_built_in();

        Array prop_list = obj->get_property_list();
        Array editable;
        for (int64_t i = 0; i < prop_list.size(); i++) {
            Dictionary p = prop_list[i];
            int usage = static_cast<int>(p.get("usage", 0));
            String pname = p.get("name", "");

            if (pname.is_empty()) continue;
            if (pname == "resource_local_to_scene") continue;
            if (pname == "resource_path") continue;
            if (pname == "resource_name") continue;
            if (!(usage & 4)) continue;

            Dictionary entry;
            entry["name"] = pname;
            entry["type"] = Variant::get_type_name(static_cast<Variant::Type>(static_cast<int>(p.get("type", 0))));
            entry["value"] = variant_to_json(obj->get(pname));
            editable.push_back(entry);
        }
        data["properties"] = editable;

        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
