#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/tile_map_layer.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot_mcp {

class SetTileMapCellsTool : public ITool {
public:
    String name() const override { return "set_tilemap_cells"; }
    String category() const override { return "editor_tools/tilemap"; }
    String brief() const override {
        return String::utf8("Batch place tiles on a TileMapLayer");
    }
    String description() const override {
        return String::utf8("Sets cells on a TileMapLayer node. Each cell specifies coordinates, "
                            "source_id, atlas_coords, and alternative_tile. Captures old cell data "
                            "before applying for undo support.");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("TileMapLayer node path");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "array";
            p["description"] = String::utf8("Array of cell objects: [{coords: [x,y], source_id: int, atlas_coords: [x,y], alternative_tile: int}]");
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

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND", String::utf8("TileMapLayer not found: ") + node_path);
        }
        godot::TileMapLayer *tilemap = godot::Object::cast_to<godot::TileMapLayer>(node);
        if (!tilemap) {
            return ToolResult::err("NOT_TILEMAP_LAYER", String::utf8("Node is not a TileMapLayer: ") + node_path);
        }

        int64_t count = cells.size();
        if (count == 0) {
            return ToolResult::err("EMPTY_CELLS", String::utf8("No cells provided"));
        }

        // Capture old cell data for undo
        godot::TypedArray<Dictionary> old_cells;
        godot::TypedArray<Dictionary> new_cells;

        auto read_coords = [](const Dictionary &d, const String &key) -> godot::Vector2i {
            if (d.has(key) && d[key].get_type() == Variant::ARRAY) {
                godot::Array a = d[key];
                if (a.size() >= 2) {
                    return godot::Vector2i((int)(int64_t)a[0], (int)(int64_t)a[1]);
                }
            }
            return godot::Vector2i();
        };

        for (int64_t i = 0; i < count; i++) {
            Dictionary cell = cells[i];

            godot::Vector2i coords = read_coords(cell, "coords");
            int64_t source_id = args_int(cell, "source_id", -1);
            godot::Vector2i atlas_coords = read_coords(cell, "atlas_coords");
            if (atlas_coords == godot::Vector2i()) {
                atlas_coords = godot::Vector2i(-1, -1);
            }
            int64_t alternative_tile = args_int(cell, "alternative_tile", 0);

            Dictionary old_cell_data;
            godot::Array arr = godot::Array::make((int64_t)coords.x, (int64_t)coords.y);
            old_cell_data["coords"] = arr;

            int64_t old_src = (int64_t)tilemap->get_cell_source_id(coords);
            bool has_old = old_src != -1;
            old_cell_data["has_cell"] = has_old;
            if (has_old) {
                old_cell_data["source_id"] = old_src;
                godot::Vector2i old_ac = tilemap->get_cell_atlas_coords(coords);
                old_cell_data["atlas_coords"] = godot::Array::make((int64_t)old_ac.x, (int64_t)old_ac.y);
                old_cell_data["alternative_tile"] = (int64_t)tilemap->get_cell_alternative_tile(coords);
            }
            old_cells.append(old_cell_data);

            tilemap->set_cell(coords, (int)source_id, atlas_coords, (int)alternative_tile);
        }

        EditorUndoRedoManager *ur = get_undo_redo();
        ur->create_action(String::utf8("MCP: Set TileMap Cells"),
                          godot::UndoRedo::MERGE_DISABLE, ctx.root);

        for (int64_t i = 0; i < count; i++) {
            Dictionary cell = cells[i];
            godot::Vector2i coords = read_coords(cell, "coords");
            int64_t source_id = args_int(cell, "source_id", -1);
            godot::Vector2i atlas_coords = read_coords(cell, "atlas_coords");
            if (atlas_coords == godot::Vector2i()) {
                atlas_coords = godot::Vector2i(-1, -1);
            }
            int64_t alternative_tile = args_int(cell, "alternative_tile", 0);
            ur->add_do_method(tilemap, "set_cell", coords, (int)source_id, atlas_coords, (int)alternative_tile);
        }

        for (int64_t i = 0; i < count; i++) {
            Dictionary oc = old_cells[i];
            godot::Vector2i coords = read_coords(oc, "coords");

            bool had_cell = args_bool(oc, "has_cell", false);
            if (had_cell) {
                int64_t old_sid = args_int(oc, "source_id", -1);
                godot::Vector2i old_ac = read_coords(oc, "atlas_coords");
                int64_t old_alt = args_int(oc, "alternative_tile", 0);
                ur->add_undo_method(tilemap, "set_cell", coords, (int)old_sid, old_ac, (int)old_alt);
            } else {
                ur->add_undo_method(tilemap, "erase_cell", coords);
            }
        }
        ur->commit_action();

        Dictionary data;
        data["cells_placed"] = count;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
