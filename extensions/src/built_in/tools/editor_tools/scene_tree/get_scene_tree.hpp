// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetSceneTreeTool : public ITool {
public:
    String name() const override { return "get_scene_tree"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("递归获取当前场景树结构");
    }
    String description() const override {
        return String::utf8("返回当前编辑场景的完整节点树，包含名称、类型、路径、子节点数、ownership 状态、脚本路径。"
                            "max_depth=-1 表示递归到叶节点。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String::utf8("最大递归深度（-1 = 无限，0 = 仅根节点，1 = 根+子节点）");
            p["default"] = (int64_t)-1;
            props["max_depth"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("是否包含每个节点的脚本路径");
            p["default"] = false;
            props["include_scripts"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        int64_t max_depth = args_int(ctx.args, "max_depth", -1);
        bool include_scripts = args_bool(ctx.args, "include_scripts", false);

        Dictionary data;
        Dictionary tree;
        scene_tree_utils::collect_node_info(
            ctx.root, ctx.root, max_depth, include_scripts, tree);

        data["root_type"] = ctx.root->get_class();
        data["root_name"] = ctx.root->get_name();
        data["scene_path"] = ctx.root->get_scene_file_path();
        data["nodes"] = tree;
        data["count"] = (int64_t)tree.size();
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
