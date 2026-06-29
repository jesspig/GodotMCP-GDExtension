#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/shader.hpp>

namespace godot_mcp {

// Coarse compatibility check between a shader uniform's declared GLSL class
// name and a Godot Variant. Returns true when the value can plausibly be
// assigned without Godot silently ignoring it. Sampler types require an
// Object/Resource; numeric scalars/vectors accept matching numeric types.
inline bool uniform_type_compatible(const godot::String &declared, const godot::Variant &value) {
    const godot::String c = declared.to_lower();
    const godot::Variant::Type vt = value.get_type();

    if (c.find("sampler") >= 0 || c.find("texture") >= 0) {
        return vt == godot::Variant::OBJECT || vt == godot::Variant::NIL;
    }
    if (c == "bool") {
        return vt == godot::Variant::BOOL || vt == godot::Variant::INT || vt == godot::Variant::FLOAT;
    }
    if (c == "int" || c == "uint" || c == "ivec2" || c == "ivec3" || c == "ivec4") {
        return vt == godot::Variant::INT || vt == godot::Variant::FLOAT;
    }
    if (c == "float") {
        return vt == godot::Variant::FLOAT || vt == godot::Variant::INT;
    }
    if (c == "vec2" || c == "vector2") return vt == godot::Variant::VECTOR2 || vt == godot::Variant::VECTOR2I;
    if (c == "vec3" || c == "vector3") return vt == godot::Variant::VECTOR3 || vt == godot::Variant::VECTOR3I;
    if (c == "vec4" || c == "vector4" || c == "quat" || c == "quaternion") {
        return vt == godot::Variant::VECTOR4 || vt == godot::Variant::VECTOR4I || vt == godot::Variant::QUATERNION;
    }
    if (c == "color") return vt == godot::Variant::COLOR;
    return true;
}

class SetShaderUniformTool : public ITool {
public:
    String name() const noexcept override { return "set_shader_uniform"; }
    String category() const noexcept override { return "editor_tools/shader"; }
    String brief() const noexcept override {
        return "Set a shader uniform parameter on a ShaderMaterial";
    }
    String description() const override {
        return "Sets a shader uniform value on a node's ShaderMaterial using "
               "set_shader_parameter(). This is the correct API for shader uniforms "
               "and cannot be covered by the generic set_node_property tool.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the node with ShaderMaterial";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Uniform parameter name";
            props["uniform_name"] = p;
        }
        {
            Dictionary p;
            p["description"] = "Value to set (type auto-converted)";
            props["value"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("node_path");
            req.append("uniform_name");
            req.append("value");
            s["required"] = req;
        }
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String uniform_name = args_string(ctx.args, "uniform_name");

        if (node_path.is_empty()) {
            return ToolResult::err("BAD_PARAM", "node_path is required");
        }
        if (uniform_name.is_empty()) {
            return ToolResult::err("BAD_PARAM", "uniform_name is required");
        }
        if (!ctx.args.has("value")) {
            return ToolResult::err("BAD_PARAM", "value is required");
        }

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        godot::ShaderMaterial *material = nullptr;
        Variant mat_var = node->get("material");
        if (mat_var.get_type() == Variant::OBJECT) {
            material = Object::cast_to<godot::ShaderMaterial>(mat_var);
        }
        if (!material) {
            return ToolResult::err("NO_SHADER_MATERIAL",
                "Node does not have a ShaderMaterial: " + node_path);
        }

        godot::Ref<godot::Shader> shader = material->get_shader();
        if (shader.is_null()) {
            return ToolResult::err("NO_SHADER",
                "ShaderMaterial has no shader assigned");
        }

        godot::StringName uniform_sn = godot::StringName(uniform_name);
        Array uniforms = shader->get_shader_uniform_list();
        bool found = false;
        String declared_class; // uniform's declared value class, if available
        for (int i = 0; i < uniforms.size(); i++) {
            Dictionary u = uniforms[i];
            if (u.get("name", "") == uniform_sn) {
                found = true;
                // get_shader_uniform_list exposes a "class_name" hint for
                // typed uniforms (e.g. "float", "vec2", "Texture2D").
                declared_class = u.get("class_name", "");
                break;
            }
        }
        if (!found) {
            return ToolResult::err("UNIFORM_NOT_FOUND",
                "Uniform not found in shader: " + uniform_name);
        }

        Variant converted = json_to_variant(ctx.args["value"]);
        // Validate the converted value's type against the uniform's declared
        // class. Silently passing a wrong type (float into a sampler, Vector2
        // into a bool) used to report success even when Godot ignored it.
        if (!declared_class.is_empty() &&
            !uniform_type_compatible(declared_class, converted)) {
            return ToolResult::err("TYPE_MISMATCH",
                String("Value type incompatible with uniform '") + uniform_name +
                String("' (declared ") + declared_class + String(")"));
        }
        Variant old_val = material->get_shader_parameter(uniform_sn);
        auto *ur = begin_undo_action("MCP: Set Shader Uniform " + uniform_name, ctx.root);
        if (ur) {
            ur->add_do_method(material, "set_shader_parameter", uniform_sn, converted);
            ur->add_undo_method(material, "set_shader_parameter", uniform_sn, old_val);
            commit_undo_action(ur);
        } else {
            material->set_shader_parameter(uniform_sn, converted);
        }
        mark_scene_dirty();

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["uniform_name"] = uniform_name;
        data["value_set"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
