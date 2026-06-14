#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot_mcp {

class SettingGetTool : public ITool {
    String name_;
    String category_;
    String setting_path_;
    String brief_;
    String cat_desc_;
    Dictionary schema_;

public:
    SettingGetTool(const String &name, const String &category,
                   const String &setting_path, const String &cat_desc = {}) :
        name_(name), category_(category),
        setting_path_(setting_path), cat_desc_(cat_desc) {
        brief_ = String("Get ") + setting_path_;
    }

    String name() const override { return name_; }
    String category() const override { return category_; }
    String brief() const override { return brief_; }
    String description() const override {
        return String("Reads the project setting \"") + setting_path_ + String("\".");
    }
    String category_description() const override { return cat_desc_; }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary input_schema() const override { return schema_; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        godot::ProjectSettings *ps = godot::ProjectSettings::get_singleton();
        Variant val = ps->get_setting(setting_path_);
        Dictionary data;
        data["setting"] = setting_path_;
        data["value"] = variant_to_json(val);
        return ToolResult::ok(data);
    }
};

class SettingSetTool : public ITool {
    String name_;
    String category_;
    String setting_path_;
    String brief_;
    String cat_desc_;
    Dictionary schema_;

public:
    SettingSetTool(const String &name, const String &category,
                   const String &setting_path, const String &cat_desc = {}) :
        name_(name), category_(category),
        setting_path_(setting_path), cat_desc_(cat_desc) {
        brief_ = String("Set ") + setting_path_;
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary vp;
        vp["description"] = String("Value for \"") + setting_path_ +
            String("\". Use native godot::JSON types (bool, number, string, array, object).");
        p["value"] = vp;
        s["properties"] = p;
        Array r;
        r.push_back("value");
        s["required"] = r;
        schema_ = s;
    }

    String name() const override { return name_; }
    String category() const override { return category_; }
    String brief() const override { return brief_; }
    String description() const override {
        return String("Sets the project setting \"") + setting_path_ +
            String("\" and persists to project.godot immediately.");
    }
    String category_description() const override { return cat_desc_; }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary input_schema() const override { return schema_; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!ctx.args.has("value")) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: value");
        }
        godot::ProjectSettings *ps = godot::ProjectSettings::get_singleton();
        Variant old_val = ps->get_setting(setting_path_);
        Variant new_val = json_to_variant(ctx.args["value"]);
        ps->set_setting(setting_path_, new_val);
        Error err = ps->save();
        if (err != OK) {
            return ToolResult::err("SAVE_FAILED",
                String("Failed to save project settings (error ") + itos(err) + String(")"));
        }
        Dictionary data;
        data["setting"] = setting_path_;
        data["previous_value"] = variant_to_json(old_val);
        data["saved"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
