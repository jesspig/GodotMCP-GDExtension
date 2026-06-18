#pragma once

#include "built_in/tool_base.hpp"

namespace godot_mcp {

class ResourceToolBase : public ITool {
public:
    String category() const noexcept override { return "node_tools/general"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }
};

} // namespace godot_mcp
