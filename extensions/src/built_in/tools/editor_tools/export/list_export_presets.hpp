
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class ListExportPresetsTool : public ITool {
public:
    String name() const override { return "list_export_presets"; }
    String category() const override { return "editor_tools/export"; }
    String brief() const override {
        return "List available export presets";
    }
    String description() const override {
        return "List all configured export presets with their platform, "
               "enabled state, and features.";
    }
    Dictionary input_schema() const override {
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
        Array results;

        for (int i = 0; i < presets.size(); i++) {
            Dictionary preset = presets[i];
            Dictionary entry;
            entry["name"] = preset.get("name", "");
            entry["platform"] = preset.get("platform", "");
            entry["enabled"] = preset.get("enabled", false);
            entry["features"] = preset.get("features", Array());
            entry["dedicated_server"] = preset.get("dedicated_server", false);
            entry["runnable"] = preset.get("runnable", false);
            results.append(entry);
        }

        Dictionary data;
        data["presets"] = results;
        data["count"] = static_cast<int64_t>(results.size());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
