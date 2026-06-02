// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class SetDisplaySettingsTool : public ITool {
public:
    String name() const override { return "set_display_settings"; }
    String category() const override { return "settings/extended"; }
    String brief() const override { return "Set display/window settings"; }
    String description() const override { return "Set display/window settings"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["viewport_width"] = [](){Dictionary d;d["type"]="integer";return d;}();
        p["viewport_height"] = [](){Dictionary d;d["type"]="integer";return d;}();
        p["mode"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["resizable"] = [](){Dictionary d;d["type"]="boolean";return d;}();
        p["borderless"] = [](){Dictionary d;d["type"]="boolean";return d;}();
        p["transparent"] = [](){Dictionary d;d["type"]="boolean";return d;}();
        p["always_on_top"] = [](){Dictionary d;d["type"]="boolean";return d;}();
        p["stretch_mode"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["stretch_aspect"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["stretch_scale"] = [](){Dictionary d;d["type"]="number";return d;}();
        p["vsync_mode"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["keep_screen_on"] = [](){Dictionary d;d["type"]="boolean";return d;}();
        p["orientation"] = [](){Dictionary d;d["type"]="string";return d;}();
        s["properties"] = p; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override { return ToolResult::ok(apply_and_save(ctx.args, display_keys())); }
};

} // namespace godot_mcp
