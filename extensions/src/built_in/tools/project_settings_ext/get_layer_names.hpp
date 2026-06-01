// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class GetLayerNamesTool : public ITool {
public:
    String name() const override { return "get_layer_names"; }
    String category() const override { return "project_settings_ext"; }
    String brief() const override { return "Get layer names for a category"; }
    String description() const override { return "Get layer names for a category"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["category"] = [](){Dictionary d;d["type"]="string";d["description"]="Layer category (2d_physics, 3d_render, etc)";return d;}();
        s["properties"] = p; Array r; r.push_back("category"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String cat = args_string(ctx.args, "category");
        if (cat.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'category'");
        int count = 0;
        if (!get_layer_count(cat, count)) return ToolResult::err("INVALID_PARAM", "unknown category '" + cat + "'");
        Dictionary layers;
        for (int i = 1; i <= count; i++) {
            String key = "layer_names/" + cat + "/layer_" + String::num_int64(i);
            Variant val = ps_get(key);
            if (val.get_type() != Variant::NIL) layers[String::num_int64(i)] = variant_to_json(val);
        }
        Dictionary d; d["category"] = cat; d["layers"] = layers;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
