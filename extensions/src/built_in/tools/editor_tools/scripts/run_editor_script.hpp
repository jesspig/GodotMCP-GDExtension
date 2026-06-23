
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/workspace/workspace_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

class RunEditorScriptTool : public ITool {
public:
    String name() const noexcept override { return "run_editor_script"; }
    String category() const noexcept override { return "editor_tools/scripts"; }
    String brief() const noexcept override {
        return "Execute an EditorScript (.gd) file in the editor context";
    }
    String description() const override {
        return "Loads and executes a GDScript file that extends EditorScript. "
               "The script's _run() method is called with full editor API access (get_scene(), EditorInterface). "
               "print() output is captured and returned as stdout. "
               "Use write_script first to create the script file, then run_editor_script to execute it. "
               "Scripts are compiled and checked by the GDScript compiler before execution.";
    }
    bool is_destructive() const override { return true; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("path", "string", "Path to .gd script file (must extend EditorScript)")
            .prop("timeout_ms", "integer", "Max execution time in milliseconds (default 5000, max 30000)",
                  static_cast<int64_t>(5000))
            .required({"path"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        if (path.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "path is required");
        }
        int64_t timeout_ms = args_int(ctx.args, "timeout_ms", 5000);
        if (timeout_ms < 100) timeout_ms = 100;
        if (timeout_ms > 30000) timeout_ms = 30000;

        // 1. Load script resource
        if (!godot::ResourceLoader::get_singleton()->exists(path)) {
            return ToolResult::err("LOAD_FAILED",
                String("Script file not found: ") + path);
        }
        godot::Ref<godot::Script> script = godot::ResourceLoader::get_singleton()->load(path, "Script");
        if (script.is_null()) {
            return ToolResult::err("LOAD_FAILED",
                String("Failed to load script: ") + path);
        }

        // 2. Instantiate via GDScript.new()
        godot::Variant instance = script->call("new");
        if (instance.get_type() != godot::Variant::OBJECT) {
            return ToolResult::err("INSTANTIATE_FAILED",
                "Script must be a valid GDScript that can be instantiated (extend EditorScript)");
        }
        godot::Object *obj = godot::Object::cast_to<godot::Object>(instance);
        if (!obj) {
            return ToolResult::err("INSTANTIATE_FAILED",
                "Script instantiation returned non-object");
        }

        // 3. Verify _run() exists
        if (!obj->has_method("_run")) {
            return ToolResult::err("NOT_EDITOR_SCRIPT",
                "Script must implement _run() method (extend EditorScript)");
        }

        // 4. Capture stdout before execution
        auto *rtl = find_console_rtl();
        String pre_text;
        if (rtl) {
            pre_text = rtl->get_text();
        }

        // 5. Execute _run()
        uint64_t start_ms = godot::Time::get_singleton()->get_ticks_msec();
        obj->call("_run");
        uint64_t elapsed_ms = godot::Time::get_singleton()->get_ticks_msec() - start_ms;

        // 6. Capture stdout after execution (delta)
        String stdout_output;
        if (rtl) {
            String post_text = rtl->get_text();
            if (post_text.length() > pre_text.length()) {
                stdout_output = post_text.substr(pre_text.length());
            }
        }

        // 7. Build result
        Dictionary data;
        data["path"] = path;
        data["execution_time_ms"] = static_cast<int64_t>(elapsed_ms);
        data["stdout"] = stdout_output;
        data["timed_out"] = static_cast<int64_t>(elapsed_ms) >= timeout_ms;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
