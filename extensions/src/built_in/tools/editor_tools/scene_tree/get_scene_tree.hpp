
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetSceneTreeTool : public ITool {
public:
    String name() const noexcept override { return "get_scene_tree"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Recursively get the current scene tree structure";
    }
    String description() const override {
        return "Returns the full node tree of the currently edited scene, including name, type, path, child count, ownership status, and script path. "
               "max_depth=-1 means recurse to leaf nodes.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("max_depth", "integer", "Maximum recursion depth (-1 = infinite, 0 = root only, 1 = root + children)", static_cast<int64_t>(-1))
            .prop("include_scripts", "boolean", "Whether to include each node's script path", false)
            .build();
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
        data["count"] = static_cast<int64_t>(tree.size());
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
