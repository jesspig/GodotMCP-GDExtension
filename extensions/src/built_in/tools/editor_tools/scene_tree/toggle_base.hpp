#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class ToggleBase : public ITool {
public:
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path")
            .prop("enable", "boolean", "true = enable, false = disable, empty = auto toggle")
            .required(Array::make("node_path"))
            .build();
    }
};

} // namespace godot_mcp
