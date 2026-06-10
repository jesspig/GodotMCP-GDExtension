// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot_mcp {

class SetThemeOverrideTool : public ITool {
public:
    String name() const override { return "set_theme_override"; }
    String category() const override { return "editor_tools/control"; }
    String brief() const override {
        return String::utf8("Batch set theme overrides on a Control node");
    }
    String description() const override {
        return String::utf8("Sets theme property overrides (color, font, font_size, stylebox, constant, icon) "
                            "on a Control node. Uses begin_bulk_theme_override/end_bulk_theme_override for batch "
                            "application. Undo restores previous values or removes new overrides.");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("Control node path");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "array";
            p["description"] = String::utf8("Array of override items: [{type, name, value}, ...]");
            Dictionary items_props;
            {
                Dictionary tp;
                tp["type"] = "string";
                tp["description"] = "Override type: color/font/font_size/stylebox/constant/icon";
                items_props["type"] = tp;
            }
            {
                Dictionary nm;
                nm["type"] = "string";
                nm["description"] = "Theme item name";
                items_props["name"] = nm;
            }
            {
                Dictionary vl;
                vl["type"] = "string";
                vl["description"] = "Theme item value (color hex, resource path, integer, etc.)";
                items_props["value"] = vl;
            }
            p["items"] = items_props;
            props["overrides"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "overrides");
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        godot::Array overrides;
        if (ctx.args.has("overrides") && ctx.args["overrides"].get_type() == Variant::ARRAY) {
            overrides = ctx.args["overrides"];
        }

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND", String::utf8("Node not found: ") + node_path);
        }
        godot::Control *control = godot::Object::cast_to<godot::Control>(node);
        if (!control) {
            return ToolResult::err("NOT_A_CONTROL", String::utf8("Node is not a Control: ") + node_path);
        }

        int64_t count = overrides.size();
        if (count == 0) {
            return ToolResult::err("EMPTY_OVERRIDES", String::utf8("No overrides provided"));
        }

        godot::TypedArray<Dictionary> undo_items;

        control->begin_bulk_theme_override();

        for (int64_t i = 0; i < count; i++) {
            Dictionary item = overrides[i];
            String override_type = args_string(item, "type");
            String override_name = args_string(item, "name");
            Variant value = item.has("value") ? item["value"] : Variant();

            Dictionary undo_item;
            undo_item["type"] = override_type;
            undo_item["name"] = override_name;

            if (override_type == "color") {
                if (control->has_theme_color_override(override_name)) {
                    undo_item["had_previous"] = true;
                    undo_item["old_value"] = control->get_theme_color(override_name);
                } else {
                    undo_item["had_previous"] = false;
                }
                godot::Color color = godot::Color::from_string(value, godot::Color());
                control->add_theme_color_override(override_name, color);

            } else if (override_type == "font") {
                if (control->has_theme_font_override(override_name)) {
                    undo_item["had_previous"] = true;
                    undo_item["old_value"] = control->get_theme_font(override_name);
                } else {
                    undo_item["had_previous"] = false;
                }
                godot::Ref<godot::Resource> font_res;
                if (value.get_type() == Variant::STRING) {
                    String s = value;
                    if (s.begins_with("res://")) {
                        font_res = godot::ResourceLoader::get_singleton()->load(s);
                    }
                }
                if (font_res.is_valid()) {
                    control->add_theme_font_override(override_name, godot::Ref<godot::Font>(font_res));
                }

            } else if (override_type == "font_size") {
                if (control->has_theme_font_size_override(override_name)) {
                    undo_item["had_previous"] = true;
                    undo_item["old_value"] = control->get_theme_font_size(override_name);
                } else {
                    undo_item["had_previous"] = false;
                }
                control->add_theme_font_size_override(override_name, (int)args_int(item, "value", 14));

            } else if (override_type == "stylebox") {
                if (control->has_theme_stylebox_override(override_name)) {
                    undo_item["had_previous"] = true;
                    undo_item["old_value"] = control->get_theme_stylebox(override_name);
                } else {
                    undo_item["had_previous"] = false;
                }
                if (value.get_type() == Variant::STRING) {
                    String s = value;
                    if (s.begins_with("res://")) {
                        godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(s);
                        if (res.is_valid()) {
                            godot::Ref<godot::StyleBox> sb = res;
                            if (sb.is_valid()) {
                                control->add_theme_stylebox_override(override_name, sb);
                            }
                        }
                    }
                }

            } else if (override_type == "constant") {
                if (control->has_theme_constant_override(override_name)) {
                    undo_item["had_previous"] = true;
                    undo_item["old_value"] = control->get_theme_constant(override_name);
                } else {
                    undo_item["had_previous"] = false;
                }
                control->add_theme_constant_override(override_name, (int)args_int(item, "value", 0));

            } else if (override_type == "icon") {
                if (control->has_theme_icon_override(override_name)) {
                    undo_item["had_previous"] = true;
                    undo_item["old_value"] = control->get_theme_icon(override_name);
                } else {
                    undo_item["had_previous"] = false;
                }
                if (value.get_type() == Variant::STRING) {
                    String s = value;
                    if (s.begins_with("res://")) {
                        godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(s);
                        if (res.is_valid()) {
                            godot::Ref<godot::Texture2D> tex = res;
                            if (tex.is_valid()) {
                                control->add_theme_icon_override(override_name, tex);
                            }
                        }
                    }
                }
            }

            undo_items.append(undo_item);
        }

        control->end_bulk_theme_override();

        EditorUndoRedoManager *ur = get_undo_redo();
        ur->create_action(String::utf8("MCP: Set Theme Overrides"),
                          godot::UndoRedo::MERGE_DISABLE, ctx.root);

        ur->add_do_method(control, "begin_bulk_theme_override");
        for (int64_t i = 0; i < count; i++) {
            Dictionary item = overrides[i];
            String override_type = args_string(item, "type");
            String override_name = args_string(item, "name");
            Variant value = item.has("value") ? item["value"] : Variant();

            if (override_type == "color") {
                godot::Color color = godot::Color::from_string(value, godot::Color());
                ur->add_do_method(control, "add_theme_color_override", override_name, color);
            } else if (override_type == "font_size") {
                ur->add_do_method(control, "add_theme_font_size_override", override_name, (int)args_int(item, "value", 14));
            } else if (override_type == "constant") {
                ur->add_do_method(control, "add_theme_constant_override", override_name, (int)args_int(item, "value", 0));
            } else if (override_type == "font") {
                if (value.get_type() == Variant::STRING) {
                    String s = value;
                    if (s.begins_with("res://")) {
                        godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(s);
                        if (res.is_valid()) {
                            ur->add_do_method(control, "add_theme_font_override", override_name, res);
                        }
                    }
                }
            } else if (override_type == "stylebox") {
                if (value.get_type() == Variant::STRING) {
                    String s = value;
                    if (s.begins_with("res://")) {
                        godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(s);
                        if (res.is_valid()) {
                            ur->add_do_method(control, "add_theme_stylebox_override", override_name, res);
                        }
                    }
                }
            } else if (override_type == "icon") {
                if (value.get_type() == Variant::STRING) {
                    String s = value;
                    if (s.begins_with("res://")) {
                        godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(s);
                        if (res.is_valid()) {
                            ur->add_do_method(control, "add_theme_icon_override", override_name, res);
                        }
                    }
                }
            }
        }
        ur->add_do_method(control, "end_bulk_theme_override");

        ur->add_undo_method(control, "begin_bulk_theme_override");
        for (int64_t i = 0; i < undo_items.size(); i++) {
            Dictionary ui = undo_items[i];
            String override_type = args_string(ui, "type");
            String override_name = args_string(ui, "name");
            bool had_previous = args_bool(ui, "had_previous", false);

            if (!had_previous) {
                if (override_type == "color") {
                    ur->add_undo_method(control, "remove_theme_color_override", override_name);
                } else if (override_type == "font") {
                    ur->add_undo_method(control, "remove_theme_font_override", override_name);
                } else if (override_type == "font_size") {
                    ur->add_undo_method(control, "remove_theme_font_size_override", override_name);
                } else if (override_type == "stylebox") {
                    ur->add_undo_method(control, "remove_theme_stylebox_override", override_name);
                } else if (override_type == "constant") {
                    ur->add_undo_method(control, "remove_theme_constant_override", override_name);
                } else if (override_type == "icon") {
                    ur->add_undo_method(control, "remove_theme_icon_override", override_name);
                }
            } else {
                Variant old_val = ui["old_value"];
                if (override_type == "color") {
                    ur->add_undo_method(control, "add_theme_color_override", override_name, old_val);
                } else {
                    ur->add_undo_method(control, "add_theme_" + override_type + "_override", override_name, old_val);
                }
            }
        }
        ur->add_undo_method(control, "end_bulk_theme_override");
        ur->commit_action();

        Dictionary data;
        data["overrides_applied"] = count;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
