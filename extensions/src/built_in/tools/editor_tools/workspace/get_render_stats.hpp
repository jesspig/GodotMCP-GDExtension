
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetRenderStatsTool : public ITool {
public:
    String name() const override { return "get_render_stats"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Get render statistics"); }
    String description() const override { return brief(); }

    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Performance *p = Performance::get_singleton();
        Dictionary d;
        d["objects_in_frame"] = p->get_monitor(Performance::RENDER_TOTAL_OBJECTS_IN_FRAME);
        d["primitives_in_frame"] = p->get_monitor(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME);
        d["draw_calls_in_frame"] = p->get_monitor(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME);
        d["video_mem_mb"] = p->get_monitor(Performance::RENDER_VIDEO_MEM_USED) / 1048576.0;
        d["texture_mem_mb"] = p->get_monitor(Performance::RENDER_TEXTURE_MEM_USED) / 1048576.0;
        d["buffer_mem_mb"] = p->get_monitor(Performance::RENDER_BUFFER_MEM_USED) / 1048576.0;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
