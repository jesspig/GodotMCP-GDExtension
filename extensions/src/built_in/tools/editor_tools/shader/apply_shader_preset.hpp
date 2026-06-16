
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class ApplyShaderPresetTool : public ITool {
public:
    String name() const noexcept override { return "apply_shader_preset"; }
    String category() const noexcept override { return "editor_tools/shader"; }
    String brief() const noexcept override {
        return "Apply a shader to a node via ShaderMaterial";
    }
    String description() const override {
        return "Apply a shader resource to a node's material slot. "
               "Creates a new ShaderMaterial if needed, or replaces "
               "the shader on an existing ShaderMaterial.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Path to the target node")
            .prop("shader_path", "string", "Path to the shader resource (.gdshader or .tres)")
            .prop("material_overwrite", "boolean", "Force create new material even if one exists")
            .required({"node_path", "shader_path"})
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        auto *ei = godot::EditorInterface::get_singleton();
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

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path_str, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        godot::Ref<godot::Shader> shader = godot::ResourceLoader::get_singleton()->load(shader_path);
        if (shader.is_null()) {
            return ToolResult::err("SHADER_NOT_FOUND",
                "Failed to load shader: " + shader_path);
        }

        bool material_created = false;
        godot::Ref<godot::ShaderMaterial> material;

        Variant existing = node->get("material");
        if (existing.get_type() != Variant::NIL && !overwrite) {
            godot::Ref<godot::ShaderMaterial> sm = existing;
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
        data["parameters"] = static_cast<int64_t>(param_count);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
