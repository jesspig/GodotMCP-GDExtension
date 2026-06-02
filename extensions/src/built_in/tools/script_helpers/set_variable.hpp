// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class SetVariableTool : public ITool {
public:
    String name() const override { return "set_variable"; }
    String category() const override { return "script_helpers"; }
    String brief() const override { return "Set a script variable value"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets a named variable/property on a node with JSON-to-Variant conversion."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["variable"] = [](){Dictionary d;d["type"]="string";d["description"]="Variable name";return d;}();
        p["value"] = [](){Dictionary d;d["type"]="object";d["description"]="Value (converted via json_to_variant)";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); r.push_back("variable"); r.push_back("value"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String var_name = args_string(ctx.args, "variable");
        if (var_name.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'variable'");
        if (!ctx.args.has("value")) return ToolResult::err("MISSING_PARAM", "missing 'value'");
        Variant gv = json_to_variant(ctx.args["value"]);
        ctx.node->set(var_name, gv);
        Variant actual = ctx.node->get(var_name);
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["variable"] = var_name;
        d["value"] = variant_to_json(actual);
        return ToolResult::ok(d);
    }
};

} // namespace
