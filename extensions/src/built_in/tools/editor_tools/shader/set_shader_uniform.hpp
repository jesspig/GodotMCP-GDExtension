
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/shader.hpp>

namespace godot_mcp {

class SetShaderUniformTool : public ITool {
public:
    String name() const override { return "set_shader_uniform"; }
    String category() const override { return "editor_tools/shader"; }
    String brief() const override {
        return "Set a shader uniform parameter on a ShaderMaterial";
    }
    String description() const override {
        return "Sets a shader uniform value on a node's ShaderMaterial using "
               "set_shader_parameter(). This is the correct API for shader uniforms "
               "and cannot be covered by the generic set_node_property tool.";
    }
    Dictionary input_schema() const override {
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

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND", "Node not found: " + node_path);
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
        for (int i = 0; i < uniforms.size(); i++) {
            Dictionary u = uniforms[i];
            if (u.get("name", "") == uniform_sn) {
                found = true;
                break;
            }
        }
        if (!found) {
            return ToolResult::err("UNIFORM_NOT_FOUND",
                "Uniform not found in shader: " + uniform_name);
        }

        Variant converted = json_to_variant(ctx.args["value"]);
        material->set_shader_parameter(uniform_sn, converted);
        mark_scene_dirty();

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["uniform_name"] = uniform_name;
        data["value_set"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
