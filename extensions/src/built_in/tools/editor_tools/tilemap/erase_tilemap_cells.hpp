#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/tile_map_layer.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot_mcp {

class EraseTileMapCellsTool : public ITool {
public:
    String name() const noexcept override { return "erase_tilemap_cells"; }
    String category() const noexcept override { return "editor_tools/tilemap"; }
    String brief() const noexcept override {
        return String("Batch erase tiles from a TileMapLayer");
    }
    String description() const override {
        return String("Erases cells from a TileMapLayer by coordinate pairs. "
                            "Captures old cell data before erasing for undo support.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("TileMapLayer node path");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "array";
            p["description"] = String("Array of [x,y] coordinate pairs to erase");
            props["cells"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "cells");
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        godot::Array cells;
        if (ctx.args.has("cells") && ctx.args["cells"].get_type() == Variant::ARRAY) {
            cells = ctx.args["cells"];
        }

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        auto *tilemap = godot::Object::cast_to<godot::TileMapLayer>(node);
        if (!tilemap) {
            return ToolResult::err("NOT_TILEMAP_LAYER", String("Node is not a TileMapLayer: ") + node_path);
        }

        int64_t count = cells.size();
        if (count == 0) {
            return ToolResult::err("EMPTY_CELLS", String("No cells provided"));
        }

        auto read_coords = [](const Dictionary &d, const String &key) -> godot::Vector2i {
            if (d.has(key) && d[key].get_type() == Variant::ARRAY) {
                godot::Array a = d[key];
                if (a.size() >= 2) {
                    return godot::Vector2i(static_cast<int>(static_cast<int64_t>(a[0])), static_cast<int>(static_cast<int64_t>(a[1])));
                }
            }
            return godot::Vector2i();
        };

        auto read_cell_arr = [](const Variant &v) -> godot::Vector2i {
            if (v.get_type() == Variant::ARRAY) {
                godot::Array a = v;
                if (a.size() >= 2) {
                    return godot::Vector2i(static_cast<int>(static_cast<int64_t>(a[0])), static_cast<int>(static_cast<int64_t>(a[1])));
                }
            }
            return godot::Vector2i();
        };

        godot::TypedArray<Dictionary> old_cells;

        for (int64_t i = 0; i < count; i++) {
            godot::Vector2i coords = read_cell_arr(cells[i]);

            Dictionary old_cell_data;
            old_cell_data["coords"] = godot::Array::make(static_cast<int64_t>(coords.x), static_cast<int64_t>(coords.y));

            int64_t old_src = static_cast<int64_t>(tilemap->get_cell_source_id(coords));
            bool has_old = old_src != -1;
            old_cell_data["has_cell"] = has_old;
            if (has_old) {
                old_cell_data["source_id"] = old_src;
                godot::Vector2i old_ac = tilemap->get_cell_atlas_coords(coords);
                old_cell_data["atlas_coords"] = godot::Array::make(static_cast<int64_t>(old_ac.x), static_cast<int64_t>(old_ac.y));
                old_cell_data["alternative_tile"] = static_cast<int64_t>(tilemap->get_cell_alternative_tile(coords));
            }
            old_cells.append(old_cell_data);
        }

        auto *ur = get_undo_redo();
        if (!ur) {
            for (int64_t i = 0; i < count; i++) {
                godot::Vector2i coords = read_cell_arr(cells[i]);
                tilemap->erase_cell(coords);
            }
            mark_scene_dirty();
        } else {
            auto *ur_action = begin_undo_action("MCP: Erase TileMap Cells");
            if (!ur_action) {
                for (int64_t i = 0; i < count; i++) {
                    godot::Vector2i coords = read_cell_arr(cells[i]);
                    tilemap->erase_cell(coords);
                }
                mark_scene_dirty();
            } else {
                for (int64_t i = 0; i < count; i++) {
                    godot::Vector2i coords = read_cell_arr(cells[i]);
                    ur_action->add_do_method(tilemap, "erase_cell", coords);
                }

                for (int64_t i = 0; i < count; i++) {
                    Dictionary oc = old_cells[i];
                    godot::Vector2i coords = read_coords(oc, "coords");

                    bool had_cell = args_bool(oc, "has_cell", false);
                    if (had_cell) {
                        int64_t old_sid = args_int(oc, "source_id", -1);
                        godot::Vector2i old_ac = read_coords(oc, "atlas_coords");
                        int64_t old_alt = args_int(oc, "alternative_tile", 0);
                        ur_action->add_undo_method(tilemap, "set_cell", coords, static_cast<int>(old_sid), old_ac, static_cast<int>(old_alt));
                    }
                }
                commit_undo_action(ur_action);
            }
        }

        Dictionary data;
        data["cells_erased"] = count;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
