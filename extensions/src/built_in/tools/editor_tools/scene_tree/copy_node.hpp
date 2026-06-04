// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

namespace godot_mcp {

class CopyNodeTool : public ITool {
public:
    String name() const override { return "copy_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("复制节点到内部剪贴板");
    }
    String description() const override {
        return String::utf8("将节点子树序列化到内部 PackedScene 剪贴板，供 paste_node 使用。"
                            "剪贴板在 Godot 进程内持续有效，跨 MCP 工具调用保持。"
                            "可携带节点和场景实例（带编辑器状态），与 Godot 编辑器内部剪贴板不互通。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("节点路径（空 = 场景根，不允许）");
            props["node_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + node_path);
        }
        if (node == ctx.root) {
            return ToolResult::err("ROOT_NOT_ALLOWED",
                String::utf8("不能复制场景根节点"));
        }

        Ref<PackedScene> scene = scene_tree_utils::pack_subtree(node);
        if (scene.is_null()) {
            return ToolResult::err("PACK_FAILED",
                String::utf8("打包节点失败"));
        }
        scene_tree_utils::set_clipboard(scene);

        Dictionary data;
        data["copied"] = relative_path(ctx.root, node);
        data["type"] = node->get_class();
        data["node_count"] = (int64_t)node->get_child_count() + 1;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
