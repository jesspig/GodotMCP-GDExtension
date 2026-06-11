
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class ApplyShaderPresetTool : public ITool {
public:
    String name() const override { return "apply_shader_preset"; }
    String category() const override { return "editor_tools/shader"; }
    String brief() const override {
        return "Apply a shader to a node via ShaderMaterial";
    }
    String description() const override {
        return "Apply a shader resource to a node's material slot. "
               "Creates a new ShaderMaterial if needed, or replaces "
               "the shader on an existing ShaderMaterial.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the target node";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the shader resource (.gdshader or .tres)";
            props["shader_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Force create new material even if one exists";
            props["material_overwrite"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("node_path");
            req.append("shader_path");
            s["required"] = req;
        }
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        String node_path_str = args_string(ctx.args, "node_path");
        String shader_path = args_string(ctx.args, "shader_path");
        bool overwrite = args_bool(ctx.args, "material_overwrite", false);

        if (node_path_str.is_empty() || shader_path.is_empty()) {
            return ToolResult::err("BAD_PARAM",
                "Both node_path and shader_path are required");
        }

        Node *node = resolve_node(ctx.root, node_path_str);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path_str);
        }

        Ref<Shader> shader = ResourceLoader::get_singleton()->load(shader_path);
        if (shader.is_null()) {
            return ToolResult::err("SHADER_NOT_FOUND",
                "Failed to load shader: " + shader_path);
        }

        bool material_created = false;
        Ref<ShaderMaterial> material;

        Variant existing = node->get("material");
        if (existing.get_type() != Variant::NIL && !overwrite) {
            Ref<ShaderMaterial> sm = existing;
            if (sm.is_valid()) {
                material = sm;
            }
        }

        if (material.is_null()) {
            material.instantiate();
            material_created = true;
        }

        material->set_shader(shader);
        node->set("material", material);

        // Count parameters via shader uniform list
        Array shader_uniforms = shader->get_shader_uniform_list();
        int param_count = shader_uniforms.size();

        Dictionary data;
        data["node_path"] = node_path_str;
        data["shader_path"] = shader_path;
        data["material_created"] = material_created;
        data["parameters"] = (int64_t)param_count;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
