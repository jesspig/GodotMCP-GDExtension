// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>

namespace godot_mcp {

class RefreshFilesystemTool : public ITool {
public:
    String name() const override { return "refresh_filesystem"; }
    String category() const override { return "editor_control"; }
    String brief() const override { return "Refresh the editor filesystem"; }
    String description() const override { return "Refresh the editor filesystem"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorFileSystem *fs = EditorInterface::get_singleton()->get_resource_filesystem();
        if (!fs) return ToolResult::err("UNAVAILABLE", "EditorFileSystem not available");
        fs->scan();
        Dictionary d; d["scanning"] = true; return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
