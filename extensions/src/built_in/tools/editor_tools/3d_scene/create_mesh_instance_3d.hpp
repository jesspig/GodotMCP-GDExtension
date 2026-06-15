
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/capsule_mesh.hpp>
#include <godot_cpp/classes/plane_mesh.hpp>
#include <godot_cpp/classes/prism_mesh.hpp>
#include <godot_cpp/classes/torus_mesh.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateMeshInstance3DTool : public ITool {
public:
    String name() const override { return "create_mesh_instance_3d"; }
    String category() const override { return "editor_tools/3d_scene"; }
    String brief() const override {
        return "Create a MeshInstance3D with a primitive or custom mesh";
    }
    String description() const override {
        return "Creates a MeshInstance3D node with a primitive shape mesh "
               "(box, sphere, cylinder, capsule, plane, torus) or loads a custom mesh/scene. "
               "For .glb/.gltf/.obj/.tscn files, instances as a PackedScene child.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Parent node path";
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Mesh type: box/sphere/cylinder/capsule/plane/torus/custom";
            props["mesh_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node name (empty = auto)";
            props["node_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "res:// path for custom mesh (.glb/.obj/.tres/.res/.tscn)";
            props["custom_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = "Size params: {width,height,depth} for box, {radius,height} for cylinder/capsule, {radius} for sphere, {size} for plane";
            props["size"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional material override (res:// path)";
            props["material_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("parent_path");
            req.append("mesh_type");
            s["required"] = req;
        }
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path");
        String mesh_type = args_string(ctx.args, "mesh_type").to_lower();
        String node_name = args_string(ctx.args, "node_name");
        String custom_path = args_string(ctx.args, "custom_path");
        String material_path = args_string(ctx.args, "material_path");

        if (mesh_type.is_empty()) {
            return ToolResult::err("BAD_PARAM", "mesh_type is required");
        }

        Node *parent = resolve_node(ctx.root, parent_path);
        if (!parent) {
            return ToolResult::err("NODE_NOT_FOUND", "Parent node not found: " + parent_path);
        }

        Dictionary size_dict;
        if (ctx.args.has("size") && ctx.args["size"].get_type() == Variant::DICTIONARY) {
            size_dict = ctx.args["size"];
        }

        static const char *scene_exts[] = {".glb", ".gltf", ".obj", ".tscn", ".scn"};
        bool is_scene_file = false;
        if (mesh_type == "custom" && !custom_path.is_empty()) {
            for (const char *ext : scene_exts) {
                if (custom_path.ends_with(ext)) {
                    is_scene_file = true;
                    break;
                }
            }
        }

        if (is_scene_file) {
            godot::Ref<godot::PackedScene> scene = godot::ResourceLoader::get_singleton()->load(custom_path);
            if (scene.is_null()) {
                return ToolResult::err("LOAD_FAILED", "Failed to load scene: " + custom_path);
            }
            Node *instance = scene->instantiate();
            if (!instance) {
                return ToolResult::err("INSTANTIATE_FAILED", "Failed to instantiate scene");
            }
            String inst_name = node_name.is_empty() ? instance->get_name() : node_name;
            instance->set_name(inst_name);

            godot::EditorUndoRedoManager *ur = get_undo_redo();
            if (!ur) {
                parent->add_child(instance, true, Node::INTERNAL_MODE_DISABLED);
                instance->set_owner(ctx.root);
                mark_scene_dirty();
            } else {
                ur->create_action("MCP: Instance scene",
                                  godot::UndoRedo::MERGE_DISABLE, ctx.root);
                ur->add_do_method(parent, "add_child", instance, true,
                                  static_cast<int64_t>(Node::INTERNAL_MODE_DISABLED));
                ur->add_do_method(instance, "set_owner", ctx.root);
                ur->add_undo_method(parent, "remove_child", instance);
                ur->add_do_reference(instance);
                ur->commit_action();
            }

            select_node(instance);

            Dictionary data;
            data["node_name"] = instance->get_name();
            data["node_path"] = relative_path(ctx.root, instance);
            data["load_type"] = "packed_scene";
            data["source_path"] = custom_path;
            return ToolResult::ok(data);
        }

        godot::MeshInstance3D *mesh_inst = memnew(godot::MeshInstance3D);
        if (!mesh_inst) {
            return ToolResult::err("CREATE_FAILED", "Failed to create MeshInstance3D");
        }

        godot::Ref<godot::Mesh> mesh;

        if (mesh_type == "custom") {
            if (custom_path.is_empty()) {
                memdelete(mesh_inst);
                return ToolResult::err("BAD_PARAM", "custom_path required for custom mesh type");
            }
            godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(custom_path);
            if (res.is_null()) {
                memdelete(mesh_inst);
                return ToolResult::err("LOAD_FAILED", "Failed to load resource: " + custom_path);
            }
            mesh = res;
            if (mesh.is_null()) {
                memdelete(mesh_inst);
                return ToolResult::err("WRONG_TYPE", "Resource is not a Mesh: " + custom_path);
            }
        } else if (mesh_type == "box") {
            godot::Ref<godot::BoxMesh> m;
            m.instantiate();
            if (!size_dict.is_empty()) {
                real_t w = static_cast<real_t>(args_float(size_dict, "width", 2.0));
                real_t h = static_cast<real_t>(args_float(size_dict, "height", 2.0));
                real_t d = static_cast<real_t>(args_float(size_dict, "depth", 2.0));
                m->set_size(godot::Vector3(w, h, d));
            }
            mesh = m;
        } else if (mesh_type == "sphere") {
            godot::Ref<godot::SphereMesh> m;
            m.instantiate();
            if (!size_dict.is_empty() && size_dict.has("radius")) {
                m->set_radius(static_cast<real_t>(args_float(size_dict, "radius", 0.5)));
                m->set_height(static_cast<real_t>(args_float(size_dict, "radius", 0.5)) * 2.0);
            }
            mesh = m;
        } else if (mesh_type == "cylinder") {
            godot::Ref<godot::CylinderMesh> m;
            m.instantiate();
            if (!size_dict.is_empty()) {
                real_t r = static_cast<real_t>(args_float(size_dict, "radius", 0.5));
                real_t h = static_cast<real_t>(args_float(size_dict, "height", 2.0));
                m->set_top_radius(r);
                m->set_bottom_radius(r);
                m->set_height(h);
            }
            mesh = m;
        } else if (mesh_type == "capsule") {
            godot::Ref<godot::CapsuleMesh> m;
            m.instantiate();
            if (!size_dict.is_empty()) {
                real_t r = static_cast<real_t>(args_float(size_dict, "radius", 0.3));
                real_t h = static_cast<real_t>(args_float(size_dict, "height", 1.0));
                m->set_radius(r);
                m->set_height(h);
            }
            mesh = m;
        } else if (mesh_type == "plane") {
            godot::Ref<godot::PlaneMesh> m;
            m.instantiate();
            if (!size_dict.is_empty() && size_dict.has("size")) {
                Variant sv = size_dict["size"];
                if (sv.get_type() == Variant::DICTIONARY) {
                    Dictionary sd = sv;
                    m->set_size(godot::Vector2(
                        static_cast<real_t>(args_float(sd, "width", 2.0)),
                        static_cast<real_t>(args_float(sd, "height", 2.0))));
                }
            }
            mesh = m;
        } else if (mesh_type == "torus") {
            godot::Ref<godot::TorusMesh> m;
            m.instantiate();
            mesh = m;
        } else {
            memdelete(mesh_inst);
            return ToolResult::err("UNKNOWN_TYPE", "Unknown mesh_type: " + mesh_type);
        }

        if (mesh.is_valid()) {
            mesh_inst->set_mesh(mesh);
        }

        if (!material_path.is_empty()) {
            godot::Ref<godot::Material> mat = godot::ResourceLoader::get_singleton()->load(material_path);
            if (mat.is_valid()) {
                mesh_inst->set_surface_override_material(0, mat);
            }
        }

        String default_name = mesh_type == "custom" ? "MeshInstance3D" : mesh_type.capitalize();
        if (node_name.is_empty()) {
            node_name = default_name;
        }
        mesh_inst->set_name(node_name);

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            parent->add_child(mesh_inst, true, Node::INTERNAL_MODE_DISABLED);
            mesh_inst->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            ur->create_action("MCP: Create MeshInstance3D",
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(parent, "add_child", mesh_inst, true,
                              static_cast<int64_t>(Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(mesh_inst, "set_owner", ctx.root);
            ur->add_undo_method(parent, "remove_child", mesh_inst);
            ur->add_do_reference(mesh_inst);
            ur->commit_action();
        }

        select_node(mesh_inst);

        Dictionary data;
        data["node_name"] = mesh_inst->get_name();
        data["node_path"] = relative_path(ctx.root, mesh_inst);
        data["mesh_type"] = mesh_type;
        data["load_type"] = "mesh_instance";
        return ToolResult::ok(data);
    }

private:
    void select_node(Node *node) {
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            godot::EditorSelection *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(node);
            }
        }
    }
};

} // namespace godot_mcp
