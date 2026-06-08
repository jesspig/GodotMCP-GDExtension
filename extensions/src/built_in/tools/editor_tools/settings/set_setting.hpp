// @tool register
// set_setting.hpp — 通用项目设置写入工具（兜底）
// 当 AI 不确定具体设置路径时使用，接受任意 setting_path + value 参数。

#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class SetSettingTool : public ITool {
public:
    String name() const override { return "set_setting"; }
    String category() const override { return "editor_tools/settings"; }
    String brief() const override {
        return "Set any project setting by path";
    }
    String description() const override {
        return "通用项目设置写入工具。接受任意 setting_path + value，"
               "修改设置并立即持久化到 project.godot。支持特性标签后缀（如 .debug、.web）。";
    }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary sp;
        sp["type"] = "string";
        sp["description"] = "Full setting path (e.g. \"application/config/name\")";
        p["setting_path"] = sp;
        Dictionary vp;
        vp["type"] = "object";
        vp["description"] = "Value for the setting (use native JSON types)";
        p["value"] = vp;
        s["properties"] = p;
        Array r;
        r.push_back("setting_path");
        r.push_back("value");
        s["required"] = r;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "setting_path");
        if (path.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "setting_path is required");
        }
        if (!ctx.args.has("value")) {
            return ToolResult::err("MISSING_PARAM", "value is required");
        }
        ProjectSettings *ps = ProjectSettings::get_singleton();
        if (!ps->has_setting(path)) {
            return ToolResult::err("SETTING_NOT_FOUND",
                String("Setting not found: ") + path);
        }
        Variant old_val = ps->get_setting(path);
        Variant new_val = json_to_variant(ctx.args["value"]);
        ps->set_setting(path, new_val);
        Error err = ps->save();
        if (err != OK) {
            return ToolResult::err("SAVE_FAILED",
                String("Failed to save project settings (error ") + itos(err) + String(")"));
        }
        Dictionary data;
        data["setting"] = path;
        data["previous_value"] = variant_to_json(old_val);
        data["new_value"] = variant_to_json(new_val);
        data["saved"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp