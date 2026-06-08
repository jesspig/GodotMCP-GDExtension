// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class InstanceChildSceneTool : public ITool {
public:
    String name() const override { return "instance_child_scene"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("实例化 .tscn 文件为场景子节点");
    }
    String description() const override {
        return String::utf8("将 .tscn 文件实例化为 parent_path 下的子节点。"
                            "instance_name 留空 = 使用 .tscn 根节点名。"
                            "editable_children=true 时子节点可在场景中编辑。"
                            "load_placeholder=true 时不展开内部结构（仅占位符）。"
                            "所有变更可撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("父节点路径（空 = 场景根）");
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要实例化的 .tscn 文件 res:// 路径");
            props["scene_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("实例节点名（留空 = 用 .tscn 根名）");
            props["instance_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("是否可编辑子节点（Allow Editable Children）");
            p["default"] = false;
            props["editable_children"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("作为占位符实例（不展开内部）");
            p["default"] = false;
            props["load_placeholder"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("scene_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path", "");
        String scene_path = args_string(ctx.args, "scene_path");
        String instance_name = args_string(ctx.args, "instance_name", "");
        bool editable_children = args_bool(ctx.args, "editable_children", false);
        bool load_placeholder = args_bool(ctx.args, "load_placeholder", false);

        if (scene_path.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("scene_path 不能为空"));
        }
        Node *parent = resolve_node(ctx.root, parent_path);
        if (!parent) {
            return ToolResult::err("PARENT_NOT_FOUND",
                String::utf8("父节点未找到: ") + parent_path);
        }

        godot::Ref<godot::PackedScene> packed =
            godot::ResourceLoader::get_singleton()->load(scene_path);
        if (packed.is_null()) {
            return ToolResult::err("LOAD_FAILED",
                String::utf8("无法加载场景: ") + scene_path);
        }
        if (!packed->can_instantiate()) {
            return ToolResult::err("NOT_INSTANTIABLE",
                String::utf8("该 PackedScene 不可实例化（可能损坏）"));
        }

        Node *inst = packed->instantiate(
            load_placeholder ? godot::PackedScene::GEN_EDIT_STATE_INSTANCE
                             : godot::PackedScene::GEN_EDIT_STATE_DISABLED);
        if (!inst) {
            return ToolResult::err("INSTANTIATE_FAILED",
                String::utf8("实例化失败"));
        }
        if (!instance_name.is_empty()) {
            inst->set_name(instance_name);
        }
        inst->set_scene_file_path(scene_path);
        inst->set_scene_instance_load_placeholder(load_placeholder);
        if (editable_children) {
            inst->set_editable_instance(inst, true);
        }
        scene_tree_utils::assign_owner_recursive(inst, ctx.root);

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Instance ") + scene_path,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(parent, "add_child", inst, true,
                              (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_undo_method(parent, "remove_child", inst);
            ur->add_do_reference(inst);
            ur->add_undo_reference(inst);
            ur->commit_action();
        } else {
            parent->add_child(inst, true, godot::Node::INTERNAL_MODE_DISABLED);
        }

        Dictionary data;
        data["instance_path"] = relative_path(ctx.root, inst);
        data["scene_path"] = scene_path;
        data["editable_children"] = editable_children;
        data["load_placeholder"] = load_placeholder;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
