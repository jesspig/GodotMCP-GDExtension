
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class GetShaderUniformsTool : public ITool {
public:
    String name() const override { return "get_shader_uniforms"; }
    String category() const override { return "editor_tools/shader"; }
    String brief() const override {
        return "Query shader uniform parameters with type info";
    }
    String description() const override {
        return "Loads a shader resource and returns all uniform parameters "
               "with their names, types, default values, and hints.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "res:// path to the shader file (.gdshader)";
            props["shader_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("shader_path");
            s["required"] = req;
        }
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String shader_path = args_string(ctx.args, "shader_path");

        if (shader_path.is_empty()) {
            return ToolResult::err("BAD_PARAM", "shader_path is required");
        }

        godot::Ref<godot::Shader> shader = godot::ResourceLoader::get_singleton()->load(shader_path);
        if (shader.is_null()) {
            return ToolResult::err("LOAD_FAILED", "Failed to load shader: " + shader_path);
        }

        Array uniforms = shader->get_shader_uniform_list();
        Array results;

        for (int i = 0; i < uniforms.size(); i++) {
            Dictionary u = uniforms[i];
            Dictionary entry;
            entry["name"] = u.get("name", "");

            int type_int = (int)u.get("type", 0);
            String type_str;
            switch (type_int) {
                case (int)Variant::BOOL: type_str = "bool"; break;
                case (int)Variant::INT: type_str = "int"; break;
                case (int)Variant::FLOAT: type_str = "float"; break;
                case (int)Variant::STRING: type_str = "String"; break;
                case (int)Variant::VECTOR2: type_str = "Vector2"; break;
                case (int)Variant::VECTOR3: type_str = "Vector3"; break;
                case (int)Variant::VECTOR4: type_str = "Vector4"; break;
                case (int)Variant::COLOR: type_str = "Color"; break;
                case (int)Variant::ARRAY: type_str = "Array"; break;
                default: type_str = String::num(type_int); break;
            }
            entry["type"] = type_str;
            entry["type_int"] = type_int;

            if (u.has("default_value")) {
                entry["default_value"] = variant_to_json(u["default_value"]);
            }

            if (u.has("hint")) {
                entry["hint"] = u["hint"];
            }
            if (u.has("hint_string")) {
                entry["hint_string"] = u["hint_string"];
            }

            results.append(entry);
        }

        Dictionary data;
        data["shader_path"] = shader_path;
        data["uniforms"] = results;
        data["count"] = (int64_t)results.size();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
