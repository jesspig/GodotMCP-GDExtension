// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/node/node_helpers.hpp"

#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

class GetScriptVariablesTool : public ITool {
public:
    String name() const override { return "get_script_variables"; }
    String category() const override { return "node"; }
    String brief() const override { return "List @export variables from a node's script"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Returns only the @export (script) variables of a node, with their current values.";
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
        Array pl = n->get_property_list();
        Array out;
        for (int i = 0; i < pl.size(); i++) {
            Dictionary d = pl[i];
            int64_t usage = (int64_t)d.get("usage", 0);
            if ((usage & 0x00001000) == 0) continue;
            if (usage & 0x00000040) continue;
            if (usage & 0x00000080) continue;
            String name = d.get("name", "");
            int64_t tid = (int64_t)d.get("type", 0);
            Dictionary e;
            e["name"] = name;
            e["type"] = variant_type_name(tid);
            e["value"] = variant_to_json(n->get(name));
            out.append(e);
        }

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, n);
        data["variables"] = out;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
