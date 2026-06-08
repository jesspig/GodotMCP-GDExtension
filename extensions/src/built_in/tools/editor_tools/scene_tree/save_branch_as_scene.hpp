// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/object.hpp>

namespace godot_mcp {

class SaveBranchAsSceneTool : public ITool {
public:
    String name() const override { return "save_branch_as_scene"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("将节点分支保存为独立的 .tscn 场景");
    }
    String description() const override {
        return String::utf8("将指定的节点及其子树打包为 PackedScene 并写入 res:// 路径。"
                            "保存后用 instance_child_scene 工具可重新实例化该分支。"
                            "本工具是结构变换——undo 不会恢复文件（仅在节点结构层面）。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("节点路径（空 = 根节点）");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标 res:// 路径（必须以 .tscn 结尾）");
            props["path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        String path = args_string(ctx.args, "path");
        if (path.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("path 不能为空"));
        }
        if (!path.ends_with(".tscn")) {
            return ToolResult::err("BAD_EXTENSION",
                String::utf8("路径必须以 .tscn 结尾"));
        }

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + node_path);
        }
        if (node == ctx.root) {
            return ToolResult::err("ROOT_NOT_ALLOWED",
                String::utf8("不能将场景根节点保存为子场景，请先选中其他节点"));
        }

        if (!ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                String::utf8("无法创建父目录: ") + path);
        }

        // 临时清空节点的 scene_file_path 以避免 .tscn 嵌套指向自身
        String old_sfp = node->get_scene_file_path();
        node->set_scene_file_path("");

        Ref<PackedScene> packed = scene_tree_utils::pack_subtree(node);
        if (packed.is_null()) {
            node->set_scene_file_path(old_sfp);
            return ToolResult::err("PACK_FAILED",
                String::utf8("打包节点失败"));
        }

        godot::Error err = godot::ResourceSaver::get_singleton()->save(packed, path);
        node->set_scene_file_path(old_sfp);
        if (err != godot::OK) {
            return ToolResult::err("SAVE_FAILED",
                String::utf8("保存失败，错误码: ") + String::num_int64((int64_t)err));
        }
        notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["node"] = relative_path(ctx.root, node);
        data["child_count"] = (int64_t)node->get_child_count();
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
