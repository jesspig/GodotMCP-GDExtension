// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class SetRenderingSettingsTool : public ITool {
public:
    String name() const override { return "set_rendering_settings"; }
    String category() const override { return "project_settings_ext"; }
    String brief() const override { return "Set rendering settings"; }
    String description() const override { return "Set rendering settings"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["rendering_method"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["default_clear_color"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["msaa_2d"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["msaa_3d"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["screen_space_aa"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["snap_2d_transforms_to_pixel"] = [](){Dictionary d;d["type"]="boolean";return d;}();
        p["snap_2d_vertices_to_pixel"] = [](){Dictionary d;d["type"]="boolean";return d;}();
        p["use_occlusion_culling"] = [](){Dictionary d;d["type"]="boolean";return d;}();
        s["properties"] = p; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override { return ToolResult::ok(apply_and_save(ctx.args, rendering_keys())); }
};

} // namespace godot_mcp
