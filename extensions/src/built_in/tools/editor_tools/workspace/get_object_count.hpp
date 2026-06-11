
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetObjectCountTool : public ITool {
public:
    String name() const override { return "get_object_count"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Get object/resource/node count"); }
    String description() const override { return brief(); }

    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Performance *p = Performance::get_singleton();
        Dictionary d;
        d["object_count"] = p->get_monitor(Performance::OBJECT_COUNT);
        d["resource_count"] = p->get_monitor(Performance::OBJECT_RESOURCE_COUNT);
        d["node_count"] = p->get_monitor(Performance::OBJECT_NODE_COUNT);
        d["orphan_node_count"] = p->get_monitor(Performance::OBJECT_ORPHAN_NODE_COUNT);
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
