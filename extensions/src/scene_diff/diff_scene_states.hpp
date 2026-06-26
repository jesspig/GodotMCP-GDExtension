#pragma once

#include "built_in/tool_base.hpp"

namespace godot_mcp {

class DiffSceneStatesTool : public ITool {
public:
    String name() const noexcept override { return "diff_scene_states"; }
    String category() const noexcept override { return "editor_tools/scene"; }
    String brief() const noexcept override {
        return "Compare two .tscn files and return their differences";
    }
    bool needs_scene() const override { return false; }

    Dictionary build_input_schema() const override;

protected:
    Dictionary execute_impl(const ToolContext &ctx) override;
};

} // namespace godot_mcp
