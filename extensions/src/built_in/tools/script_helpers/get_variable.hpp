// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class GetVariableTool : public ITool {
public:
    String name() const override { return "get_variable"; }
    String category() const override { return "script_helpers"; }
    String brief() const override { return "Get a script variable value"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Reads a named variable/property from a node."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["variable"] = [](){Dictionary d;d["type"]="string";d["description"]="Variable name";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); r.push_back("variable"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String var_name = args_string(ctx.args, "variable");
        if (var_name.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'variable'");
        Variant val = ctx.node->get(var_name);
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["variable"] = var_name;
        d["value"] = variant_to_json(val); d["type"] = (int64_t)val.get_type();
        return ToolResult::ok(d);
    }
};

} // namespace
