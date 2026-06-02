// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class SetLayerNamesTool : public ITool {
public:
    String name() const override { return "set_layer_names"; }
    String category() const override { return "settings/extended"; }
    String brief() const override { return "Set layer names for a category"; }
    String description() const override { return "Set layer names for a category"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["category"] = [](){Dictionary d;d["type"]="string";d["description"]="Layer category";return d;}();
        p["layers"] = [](){Dictionary d;d["type"]="object";d["description"]="Map of layer numbers to names";return d;}();
        s["properties"] = p; Array r; r.push_back("category"); r.push_back("layers"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String cat = args_string(ctx.args, "category");
        if (cat.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'category'");
        if (!ctx.args.has("layers") || ctx.args["layers"].get_type() != Variant::DICTIONARY)
            return ToolResult::err("MISSING_PARAM", "missing 'layers' object");
        Dictionary layers = ctx.args["layers"];
        int count = 0;
        if (!get_layer_count(cat, count)) return ToolResult::err("INVALID_PARAM", "unknown category '" + cat + "'");
        int updated = 0;
        for (int i = 1; i <= count; i++) {
            String idx = String::num_int64(i);
            if (layers.has(idx)) { ps_set("layer_names/" + cat + "/layer_" + idx, json_to_variant(layers[idx])); updated++; }
        }
        Dictionary d; d["category"] = cat; d["updated"] = (int64_t)updated;
        if (!ps_save()) d["warning"] = "disk write failed";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
