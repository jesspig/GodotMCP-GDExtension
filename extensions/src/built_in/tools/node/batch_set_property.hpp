// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

class BatchSetPropertyTool : public ITool {
public:
    String name() const override { return "batch_set_property"; }
    String category() const override { return "node"; }
    String brief() const override { return "Set a property on multiple nodes at once"; }
    bool needs_scene() const override { return true; }
    String description() const override {
        return "Applies a property change to every node listed in node_paths. "
               "Each node is processed independently; failures are reported per-node.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_paths"] = []() { Dictionary d; d["type"] = "array"; d["items"] = Dictionary(); d["description"] = "Array of node paths"; return d; }();
        props["property"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Property name to set"; return d; }();
        props["value"] = []() { Dictionary d; d["type"] = "object"; d["description"] = "Value to set"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_paths"); req.push_back("property"); req.push_back("value"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!ctx.args.has("node_paths") || ctx.args["node_paths"].get_type() != Variant::ARRAY)
            return ToolResult::err("MISSING_PARAM", "missing 'node_paths' array");

        Array paths = ctx.args["node_paths"];
        String prop = args_string(ctx.args, "property");
        if (prop.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'property'");
        if (!ctx.args.has("value")) return ToolResult::err("MISSING_PARAM", "missing 'value'");

        Variant gv = json_to_variant(ctx.args["value"]);
        Node *root = ctx.root;
        Array results;

        for (int i = 0; i < paths.size(); i++) {
            String pi = (String)paths[i];
            Node *n = resolve_node(root, pi);
            Dictionary e;
            e["node_path"] = pi;
            if (n) {
                undoable_set(n, prop, gv, "Batch set " + prop + " for " + pi);
                e["status"] = "ok";
                e["value"] = variant_to_json(n->get(prop));
            } else {
                e["status"] = "error";
                e["message"] = "not found";
            }
            results.append(e);
        }

        Dictionary data;
        data["results"] = results;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
