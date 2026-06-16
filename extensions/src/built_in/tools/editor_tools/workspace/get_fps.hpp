#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetFpsTool : public ITool {
public:
    String name() const noexcept override { return "get_fps"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Get current FPS"); }
    String description() const override { return brief(); }

    Dictionary build_input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        double fps = godot::Performance::get_singleton()->get_monitor(godot::Performance::TIME_FPS);
        Dictionary d;
        d["fps"] = fps;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
