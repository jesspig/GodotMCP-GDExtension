// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class GetOpenScenesTool : public ITool {
public:
    String name() const override { return "get_open_scenes"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Get list of open scene paths"; }
    String description() const override {
        return "Returns an array of all scene file paths currently open in the editor.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        schema["properties"] = Dictionary();
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        PackedStringArray paths = EditorInterface::get_singleton()->get_open_scenes();
        Array out;
        for (int i = 0; i < paths.size(); i++) {
            Dictionary e;
            e["path"] = paths[i];
            out.append(e);
        }
        Dictionary data;
        data["scenes"] = out;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
