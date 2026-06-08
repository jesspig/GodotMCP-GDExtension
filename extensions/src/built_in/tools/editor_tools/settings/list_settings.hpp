// @tool register
// list_settings.hpp — 列举所有项目设置，支持按分类过滤和搜索

#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class ListSettingsTool : public ITool {
public:
    String name() const override { return "list_settings"; }
    String category() const override { return "editor_tools/settings"; }
    String brief() const override {
        return "List all project settings, optionally filtered";
    }
    String description() const override {
        return "列举所有项目设置（包含特性标签变体）。支持按分类前缀过滤（如 \"display/window\"）"
               "和文本搜索。返回设置名称、类型、当前值。";
    }
    bool is_meta() const override { return true; }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary fp;
        fp["type"] = "string";
        fp["description"] = "Optional category filter (e.g. \"display/window\")";
        p["filter"] = fp;
        Dictionary sp;
        sp["type"] = "string";
        sp["description"] = "Optional search text";
        p["search"] = sp;
        Dictionary lp;
        lp["type"] = "integer";
        lp["description"] = "Max results (default 200)";
        p["limit"] = lp;
        s["properties"] = p;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String filter = args_string(ctx.args, "filter");
        String search = args_string(ctx.args, "search").to_lower();
        int64_t limit = args_int(ctx.args, "limit", 200);
        if (limit <= 0) limit = 200;
        if (limit > 5000) limit = 5000;

        ProjectSettings *ps = ProjectSettings::get_singleton();
        Array prop_list = ps->get_property_list();

        Array results;
        int count = 0;
        for (int i = 0; i < prop_list.size() && count < limit; i++) {
            Dictionary prop = prop_list[i];
            String name = prop["name"];

            if (!filter.is_empty() && !name.begins_with(filter)) {
                continue;
            }
            if (!search.is_empty() && name.to_lower().find(search) == -1) {
                continue;
            }
            // Skip editor_settings_override entries in general listing
            if (name.begins_with("editor_settings_override/")) {
                continue;
            }

            int usage = prop.get("usage", 0);
            // Skip settings without EDITOR flag
            if ((usage & 4) == 0) {
                continue;
            }

            Dictionary entry;
            entry["setting"] = name;
            entry["type"] = prop.get("type", 0);

            Variant val = ps->get_setting(name);
            entry["value"] = variant_to_json(val);

            bool basic = (usage & 256) != 0;
            bool restart = (usage & 2048) != 0;
            entry["basic"] = basic;
            entry["restart_if_changed"] = restart;

            results.push_back(entry);
            count++;
        }

        Dictionary data;
        data["count"] = count;
        data["total_matched"] = count;
        data["settings"] = results;
        data["filter"] = filter.is_empty() ? Variant() : Variant(filter);
        data["search"] = search.is_empty() ? Variant() : Variant(search);
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp