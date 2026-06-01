// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SaveAllScenesTool : public ITool {
public:
    String name() const override { return "save_all_scenes"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Save all open scenes"; }
    String description() const override {
        return "Saves every open scene in the editor and records save version markers.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        schema["properties"] = Dictionary();
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        int64_t before = ei->get_open_scenes().size();
        ei->save_all_scenes();

        Array roots = ei->get_open_scene_roots();
        for (int i = 0; i < roots.size(); i++) {
            Node *rn = Object::cast_to<Node>(roots[i]);
            if (rn) save_version_marker(rn);
        }

        Dictionary data;
        data["count"] = before;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
