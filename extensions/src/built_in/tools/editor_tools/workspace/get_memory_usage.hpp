
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetMemoryUsageTool : public ITool {
public:
    String name() const override { return "get_memory_usage"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Get memory usage"); }
    String description() const override { return brief(); }

    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Performance *p = Performance::get_singleton();
        Dictionary d;
        d["memory_static_mb"] = p->get_monitor(Performance::MEMORY_STATIC) / 1048576.0;
        d["memory_static_max_mb"] = p->get_monitor(Performance::MEMORY_STATIC_MAX) / 1048576.0;
        d["message_buffer_max_mb"] = p->get_monitor(Performance::MEMORY_MESSAGE_BUFFER_MAX) / 1048576.0;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
