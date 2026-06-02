// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot_mcp {

class GodotEditorRestartTool : public ITool {
public:
    String name() const override { return "godot_editor_restart"; }
    String category() const override { return "editor_control"; }
    String brief() const override { return "Restart the Godot editor"; }
    String description() const override { return "Restart the Godot editor"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override {
        OS *os = OS::get_singleton();
        const String exe = os->get_executable_path();
        const String project = globalize_path("res://");
        PackedStringArray args;
        args.push_back("--editor"); args.push_back("--path"); args.push_back(project);
        int32_t pid = os->create_process(exe, args);
        if (pid < 0) return ToolResult::err("LAUNCH_FAILED", "Failed to launch editor (err=" + String::num_int64(pid) + ")");
        EditorInterface::get_singleton()->get_base_control()->get_tree()->quit();
        Dictionary d; d["restarting"] = true; return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
