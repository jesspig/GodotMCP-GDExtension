
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"
#include "built_in/cmd_utils/dispatch_map.hpp"
#include "built_in/cmd_utils/memdelete_guard.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

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
    String name() const noexcept override { return "create_mesh_instance_3d"; }
    String category() const noexcept override { return "editor_tools/3d_scene"; }
    String brief() const noexcept override {
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

        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        Dictionary size_dict = args_get_typed<Dictionary>(ctx.args, "size", Dictionary());

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

            auto *ur = begin_undo_action("MCP: Instance scene");
            if (!ur) {
                parent->add_child(instance, true, Node::INTERNAL_MODE_DISABLED);
                instance->set_owner(ctx.root);
                mark_scene_dirty();
            } else {
                commit_add_child_undo(ur, "MCP: Instance scene", parent, instance, ctx.root);
            }

            select_node(instance);

            Dictionary data;
            data["node_name"] = instance->get_name();
            data["node_path"] = relative_path(ctx.root, instance);
            data["load_type"] = "packed_scene";
            data["source_path"] = custom_path;
            return ToolResult::ok(data);
        }

        auto *mesh_inst = memnew(godot::MeshInstance3D);
        if (!mesh_inst) {
            return ToolResult::err("CREATE_FAILED", "Failed to create MeshInstance3D");
        }
        MemdeleteGuard<godot::MeshInstance3D> guard(mesh_inst);

        godot::Ref<godot::Mesh> mesh;

        if (mesh_type == "custom") {
            if (custom_path.is_empty()) {
                return ToolResult::err("BAD_PARAM", "custom_path required for custom mesh type");
            }
            godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(custom_path);
            if (res.is_null()) {
                return ToolResult::err("LOAD_FAILED", "Failed to load resource: " + custom_path);
            }
            mesh = res;
            if (mesh.is_null()) {
                return ToolResult::err("WRONG_TYPE", "Resource is not a Mesh: " + custom_path);
            }
        } else {
            static const auto kMeshClasses = godot_mcp::make_dispatch_map<godot::String, godot::String>(
                std::pair{godot::String("box"),     godot::String("BoxMesh")},
                std::pair{godot::String("sphere"),  godot::String("SphereMesh")},
                std::pair{godot::String("cylinder"),godot::String("CylinderMesh")},
                std::pair{godot::String("capsule"), godot::String("CapsuleMesh")},
                std::pair{godot::String("plane"),   godot::String("PlaneMesh")},
                std::pair{godot::String("torus"),   godot::String("TorusMesh")}
            );
            assert(kMeshClasses.validate() && "Duplicate key");

            const godot::String* matched = kMeshClasses.find(godot::String(mesh_type));
            if (!matched) {
                return ToolResult::err("UNKNOWN_TYPE", "Unknown mesh_type: " + mesh_type);
            }

            godot::String class_name = *matched;
            Object *mesh_obj = ClassDB::instantiate(class_name);
            if (!mesh_obj) {
                return ToolResult::err("CREATE_FAILED", "Failed to create " + class_name);
            }
            mesh = godot::Ref<godot::Mesh>(Object::cast_to<godot::Mesh>(mesh_obj));
            if (mesh.is_null()) {
                return ToolResult::err("CLASS_NOT_AVAILABLE",
                    class_name + " is not available in this mode (headless rendering disabled)");
            }

            if (mesh_type == "box") {
                auto *m = Object::cast_to<godot::BoxMesh>(mesh.ptr());
                if (m && !size_dict.is_empty()) {
                    m->set_size(godot::Vector3(
                        static_cast<real_t>(args_float(size_dict, "width", 2.0)),
                        static_cast<real_t>(args_float(size_dict, "height", 2.0)),
                        static_cast<real_t>(args_float(size_dict, "depth", 2.0))));
                }
            } else if (mesh_type == "sphere") {
                auto *m = Object::cast_to<godot::SphereMesh>(mesh.ptr());
                if (m && !size_dict.is_empty() && size_dict.has("radius")) {
                    real_t r = static_cast<real_t>(args_float(size_dict, "radius", 0.5));
                    m->set_radius(r);
                    m->set_height(r * 2.0f);
                }
            } else if (mesh_type == "cylinder") {
                auto *m = Object::cast_to<godot::CylinderMesh>(mesh.ptr());
                if (m && !size_dict.is_empty()) {
                    real_t r = static_cast<real_t>(args_float(size_dict, "radius", 0.5));
                    real_t h = static_cast<real_t>(args_float(size_dict, "height", 2.0));
                    m->set_top_radius(r);
                    m->set_bottom_radius(r);
                    m->set_height(h);
                }
            } else if (mesh_type == "capsule") {
                auto *m = Object::cast_to<godot::CapsuleMesh>(mesh.ptr());
                if (m && !size_dict.is_empty()) {
                    real_t r = static_cast<real_t>(args_float(size_dict, "radius", 0.3));
                    real_t h = static_cast<real_t>(args_float(size_dict, "height", 1.0));
                    m->set_radius(r);
                    m->set_height(h);
                }
            } else if (mesh_type == "plane") {
                auto *m = Object::cast_to<godot::PlaneMesh>(mesh.ptr());
                if (m && !size_dict.is_empty() && size_dict.has("size")) {
                    Variant sv = size_dict["size"];
                    if (sv.get_type() == Variant::DICTIONARY) {
                        Dictionary sd = sv;
                        m->set_size(godot::Vector2(
                            static_cast<real_t>(args_float(sd, "width", 2.0)),
                            static_cast<real_t>(args_float(sd, "height", 2.0))));
                    }
                }
            }
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

        auto *ur = begin_undo_action("MCP: Create MeshInstance3D");
        if (!ur) {
            parent->add_child(mesh_inst, true, Node::INTERNAL_MODE_DISABLED);
            mesh_inst->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            commit_add_child_undo(ur, "MCP: Create MeshInstance3D", parent, mesh_inst, ctx.root);
        }
        guard.dismiss();

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
        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(node);
            }
        }
    }
};

} // namespace godot_mcp
