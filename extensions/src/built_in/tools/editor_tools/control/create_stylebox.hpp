
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

namespace godot_mcp {

class CreateStyleBoxTool : public ITool {
public:
    String name() const override { return "create_stylebox"; }
    String category() const override { return "editor_tools/control"; }
    String brief() const override {
        return String("Create a StyleBoxFlat resource and optionally apply as theme override");
    }
    String description() const override {
        return String("Creates a StyleBoxFlat resource with configurable background color, "
                            "border color, border width, and corner radius. Can optionally apply "
                            "it as a theme stylebox override on a Control node.");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("StyleBox name (used as theme override name)");
            props["stylebox_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Optional: node path to apply the stylebox to");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Background color hex (e.g. #ffffff)");
            p["default"] = "#ffffff";
            props["bg_color"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Border color hex (e.g. #000000)");
            p["default"] = "#000000";
            props["border_color"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String("Border width in pixels");
            p["default"] = 0;
            props["border_width"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String("Corner radius in pixels");
            p["default"] = 0;
            props["corner_radius"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String("Apply as theme override on the target node");
            p["default"] = false;
            props["apply_to_node"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("stylebox_name");
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String stylebox_name = args_string(ctx.args, "stylebox_name");
        String node_path = args_string(ctx.args, "node_path");
        String bg_color_str = args_string(ctx.args, "bg_color", "#ffffff");
        String border_color_str = args_string(ctx.args, "border_color", "#000000");
        int64_t border_width = args_int(ctx.args, "border_width", 0);
        int64_t corner_radius = args_int(ctx.args, "corner_radius", 0);
        bool apply_to_node = args_bool(ctx.args, "apply_to_node", false);

        godot::Color bg = godot::Color::from_string(bg_color_str, godot::Color(1, 1, 1));
        godot::Color border = godot::Color::from_string(border_color_str, godot::Color(0, 0, 0));

        godot::Ref<godot::StyleBoxFlat> sb;
        sb.instantiate();
        sb->set_bg_color(bg);
        sb->set_border_color(border);
        sb->set_border_width_all((int)border_width);
        sb->set_corner_radius_all((int)corner_radius);

        godot::Control *control = nullptr;
        if (apply_to_node && !node_path.is_empty()) {
            Node *node = resolve_node(ctx.root, node_path);
            if (!node) {
                return ToolResult::err("NODE_NOT_FOUND", String("Node not found: ") + node_path);
            }
            control = godot::Object::cast_to<godot::Control>(node);
            if (!control) {
                return ToolResult::err("NOT_A_CONTROL", String("Node is not a Control: ") + node_path);
            }

            godot::EditorUndoRedoManager *ur = get_undo_redo();
            if (!ur) {
                control->add_theme_stylebox_override(stylebox_name, sb);
                mark_scene_dirty();
            } else {
                ur->create_action(String("MCP: Create StyleBox ") + stylebox_name,
                                  godot::UndoRedo::MERGE_DISABLE, ctx.root);

                bool had_override = control->has_theme_stylebox_override(stylebox_name);
                godot::Variant old_sb;
                if (had_override) {
                    old_sb = control->get_theme_stylebox(stylebox_name);
                }

                ur->add_do_method(control, "add_theme_stylebox_override", stylebox_name, sb);
                if (had_override) {
                    ur->add_undo_method(control, "add_theme_stylebox_override", stylebox_name, old_sb);
                } else {
                    ur->add_undo_method(control, "remove_theme_stylebox_override", stylebox_name);
                }
                ur->commit_action();
            }
        }

        Dictionary data;
        data["stylebox_created"] = true;
        data["bg_color"] = bg_color_str;
        data["border_color"] = border_color_str;
        data["border_width"] = border_width;
        data["corner_radius"] = corner_radius;
        data["applied_to_node"] = apply_to_node;
        if (control) {
            data["node_path"] = node_path;
        }
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

