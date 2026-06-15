
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class ReadShaderTool : public ITool {
public:
    String name() const noexcept override { return "read_shader"; }
    String category() const noexcept override { return "editor_tools/shader"; }
    String brief() const noexcept override {
        return "Read an existing shader resource's code";
    }
    String description() const override {
        return "Load a shader resource and return its shader type (mode), "
               "source code, and uniform count.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the shader resource";
            props["resource_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("resource_path");
            s["required"] = req;
        }
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String resource_path = args_string(ctx.args, "resource_path");

        if (resource_path.is_empty()) {
            return ToolResult::err("BAD_PARAM", "resource_path is required");
        }

        godot::Ref<godot::Shader> shader = godot::ResourceLoader::get_singleton()->load(resource_path);
        if (shader.is_null()) {
            return ToolResult::err("LOAD_FAILED",
                "Failed to load shader: " + resource_path);
        }

        String code = shader->get_code();

        // Determine shader mode from code
        String mode = "unknown";
        if (code.begins_with("shader_type spatial")) {
            mode = "spatial";
        } else if (code.begins_with("shader_type canvas_item")) {
            mode = "canvas_item";
        } else if (code.begins_with("shader_type particles")) {
            mode = "particles";
        } else if (code.begins_with("shader_type sky")) {
            mode = "sky";
        } else if (code.begins_with("shader_type fog")) {
            mode = "fog";
        }

        // Count uniforms in code
        int uniform_count = 0;
        int pos = 0;
        while (true) {
            pos = code.find("uniform ", pos);
            if (pos == -1) break;
            // Skip comments
            int line_start = code.rfind("\n", pos);
            if (line_start >= 0) {
                int comment_pos = code.find("//", line_start + 1);
                if (comment_pos >= 0 && comment_pos < pos) {
                    pos += 8;
                    continue;
                }
            }
            uniform_count++;
            pos += 8;
        }

        Dictionary data;
        data["resource_path"] = resource_path;
        data["mode"] = mode;
        data["code"] = code;
        data["uniform_count"] = static_cast<int64_t>(uniform_count);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
