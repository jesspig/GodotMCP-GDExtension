
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class GetExportPlatformsTool : public ITool {
public:
    String name() const override { return "get_export_platforms"; }
    String category() const override { return "editor_tools/export"; }
    String brief() const override {
        return "List available export platforms and template status";
    }
    String description() const override {
        return "Lists export platforms referenced by configured presets, "
               "showing platform name, preset count, and basic status.";
    }
    Dictionary build_input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        Array presets = ei->call("get_export_presets");

        Dictionary platforms;
        for (int i = 0; i < presets.size(); i++) {
            Dictionary preset = presets[i];
            String platform = preset.get("platform", "");
            if (platform.is_empty()) continue;

            if (!platforms.has(platform)) {
                Dictionary info;
                info["name"] = platform;
                info["preset_count"] = (int64_t)0;
                info["has_runnable"] = false;
                platforms[platform] = info;
            }

            Dictionary info = platforms[platform];
            info["preset_count"] = static_cast<int64_t>(info["preset_count"]) + 1;
            if (preset.get("runnable", false)) {
                info["has_runnable"] = true;
            }
            platforms[platform] = info;
        }

        Array results;
        Array keys = platforms.keys();
        for (int i = 0; i < keys.size(); i++) {
            results.append(platforms[keys[i]]);
        }

        Dictionary data;
        data["platforms"] = results;
        data["count"] = static_cast<int64_t>(results.size());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
