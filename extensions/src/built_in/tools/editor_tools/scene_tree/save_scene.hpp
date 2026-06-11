#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SaveSceneTool : public ITool {
public:
    String name() const override { return "save_scene"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Save the current edited scene to a res:// path";
    }
    String description() const override {
        return "Saves the currently open scene to disk. If path is empty, saves to the original .tscn path. "
               "Returns an error if the scene was never saved and path is empty.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "res:// path (empty = save to original path, must end with .tscn/.scn)";
            props["path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        if (path.is_empty()) {
            // Capture path BEFORE save â€?ei->save_scene() may reload the scene
            // and invalidate ctx.root (dangling pointer â†?crash).
            path = ctx.root->get_scene_file_path();
            if (path.is_empty()) {
                return ToolResult::err("NO_PATH",
                    "Scene was never saved and no path argument provided");
            }
            // Use save_scene_as(path, false) directly to bypass EditorProgress
            // (_save_scene_with_preview). EditorProgress::step() calls
            // Main::iteration() internally, which triggers a recursive
            // _process() â†?http_server_.poll() and causes crashes.
            ei->save_scene_as(path, false);
        } else {
            if (!path.ends_with(".tscn") && !path.ends_with(".scn")) {
                return ToolResult::err("BAD_EXTENSION",
                    "Path must end with .tscn or .scn");
            }
            if (!ensure_parent_dir(path)) {
                return ToolResult::err("MKDIR_FAILED",
                    "Failed to create parent directory: " + path);
            }
            ei->save_scene_as(path, false);
        }

        Dictionary data;
        data["path"] = path;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

