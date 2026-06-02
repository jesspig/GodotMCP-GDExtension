// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace godot_mcp {

class GetEditorInfoTool : public ITool {
public:
    String name() const override { return "get_editor_info"; }
    String category() const override { return "editor_control"; }
    String brief() const override { return "Get editor info (version, scale, paths)"; }
    String category_description() const override { return "Editor control and info query"; }
    String description() const override { return "Get editor info (version, scale, paths)"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        Engine *eng = Engine::get_singleton();
        Dictionary vi = eng->get_version_info();
        String ev = (String)vi.get("string", "");
        double scale = ei->get_editor_scale();
        EditorPaths *paths = ei->get_editor_paths();
        Dictionary d;
        d["engine_version"] = ev; d["editor_scale"] = scale;
        if (paths) {
            Dictionary pd; pd["data_dir"] = paths->get_data_dir(); pd["config_dir"] = paths->get_config_dir();
            pd["cache_dir"] = paths->get_cache_dir(); pd["project_settings_dir"] = paths->get_project_settings_dir();
            d["editor_paths"] = pd;
        }
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
