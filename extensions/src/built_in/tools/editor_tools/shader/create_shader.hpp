
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

namespace godot_mcp {

class CreateShaderTool : public ITool {
public:
    String name() const override { return "create_shader"; }
    String category() const override { return "editor_tools/shader"; }
    String brief() const override {
        return "Create a new shader resource file";
    }
    String description() const override {
        return "Create a new shader (.gdshader or .gdshaderinc) resource "
               "with the specified mode and optional code template.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Resource path (e.g. res://shaders/my_shader.gdshader)";
            props["resource_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Shader mode: spatial, canvas_item, particles, sky, or fog";
            props["mode"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Custom shader code (optional, default template)";
            props["code"] = p;
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
        String mode = args_string(ctx.args, "mode", "canvas_item");
        String code = args_string(ctx.args, "code");

        if (resource_path.is_empty()) {
            return ToolResult::err("BAD_PARAM", "resource_path is required");
        }

        if (code.is_empty()) {
            if (mode == "spatial") {
                code = "shader_type spatial;\n\nvoid fragment() {\n\tALBEDO = vec3(1.0);\n}\n";
            } else if (mode == "canvas_item") {
                code = "shader_type canvas_item;\n\nvoid fragment() {\n\tCOLOR = vec4(1.0);\n}\n";
            } else if (mode == "particles") {
                code = "shader_type particles;\n\nvoid process() {\n\tTRANSFORM = TRANSFORM * TRANSFORM;\n}\n";
            } else if (mode == "sky") {
                code = "shader_type sky;\n\nvoid sky() {\n\tCOLOR = vec4(0.5, 0.6, 1.0, 1.0);\n}\n";
            } else if (mode == "fog") {
                code = "shader_type fog;\n\nvoid fog() {\n\tCOLOR = vec4(0.5, 0.6, 0.7, 1.0);\n}\n";
            } else {
                return ToolResult::err("BAD_PARAM",
                    "Unknown mode: " + mode + ". Use spatial, canvas_item, particles, sky, or fog.");
            }
        }

        ensure_parent_dir(resource_path);

        godot::Ref<godot::Shader> shader;
        shader.instantiate();
        shader->set_code(code);

        Error err = godot::ResourceSaver::get_singleton()->save(shader, resource_path);
        if (err != godot::OK) {
            return ToolResult::err("SAVE_FAILED",
                "Failed to save shader: " + resource_path);
        }

        notify_file_changed(resource_path);

        Dictionary data;
        data["resource_path"] = resource_path;
        data["mode"] = mode;
        data["saved"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
