// @tool register
#pragma once

#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class GetOpenSceneRootsTool : public ITool {
public:
    String name() const override { return "get_open_scene_roots"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Get root nodes of all open scenes"; }
    String description() const override {
        return "Returns the root nodes of every open scene tab, including name, class, and scene file path.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        schema["properties"] = Dictionary();
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Array roots = EditorInterface::get_singleton()->get_open_scene_roots();
        Array out;
        for (int i = 0; i < roots.size(); i++) {
            Node *rn = Object::cast_to<Node>(roots[i]);
            if (!rn) continue;
            String sf = rn->get_scene_file_path();
            Dictionary e;
            e["name"] = rn->get_name();
            e["class"] = rn->get_class();
            e["scene_file_path"] = sf.is_empty() ? rn->get_name() : sf;
            out.append(e);
        }
        Dictionary data;
        data["roots"] = out;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
