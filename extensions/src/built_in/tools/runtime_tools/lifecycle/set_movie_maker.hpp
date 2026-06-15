
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SetMovieMakerTool : public ITool {
public:
    String name() const override { return "set_movie_maker"; }
    String category() const override { return "runtime_tools/lifecycle"; }
    String brief() const override { return String("Enable or disable Movie Maker (recording) mode"); }
    String description() const override {
        return String("Enables or disables Godot's Movie Maker (recording) mode. When enabled, running the project will automatically record a video file. "
                             "Output path and related settings are configured in ProjectSettings under editor/movie_writer/*. "
                             "Equivalent to the Project > Enable Movie Maker menu item in the editor. "
                             "Use is_movie_maker_enabled to query the current state.");
    }
    Dictionary build_input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "boolean";
            d["description"] = String("Whether to enable Movie Maker mode");
            p["enabled"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("enabled");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }
        bool enabled = args_bool(ctx.args, "enabled");
        // Capture the previous state BEFORE mutation; reading it after
        // set_movie_maker_enabled() always returns the new value.
        bool was_enabled = ei->is_movie_maker_enabled();
        ei->set_movie_maker_enabled(enabled);
        Dictionary data;
        data["action"] = "set_movie_maker_enabled";
        data["previous"] = was_enabled;
        data["current"] = enabled;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

