
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/tile_map_layer.hpp>
#include <godot_cpp/classes/tile_set.hpp>
#include <godot_cpp/classes/tile_set_source.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot_mcp {

class GetTileMapInfoTool : public ITool {
public:
    String name() const noexcept override { return "get_tilemap_info"; }
    String category() const noexcept override { return "editor_tools/tilemap"; }
    String brief() const noexcept override {
        return String("Query TileMapLayer metadata and cell data");
    }
    String description() const override {
        return String("Returns metadata about a TileMapLayer: enabled state, tile set info, "
                            "used cells count and positions, used rect, collision/navigation state, "
                            "y_sort_origin, and rendering quadrant size. Read-only, no undo needed.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("TileMapLayer node path");
            props["node_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path");
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        auto *tilemap = godot::Object::cast_to<godot::TileMapLayer>(node);
        if (!tilemap) {
            return ToolResult::err("NOT_TILEMAP_LAYER", String("Node is not a TileMapLayer: ") + node_path);
        }

        Dictionary data;
        data["enabled"] = tilemap->is_enabled();

        // TileSet info
        Dictionary ts_info;
        godot::Ref<godot::TileSet> tile_set = tilemap->get_tile_set();
        if (tile_set.is_valid()) {
            int64_t source_count = tile_set->get_source_count();
            ts_info["source_count"] = source_count;

            godot::Array sources;
            for (int64_t i = 0; i < source_count; i++) {
                int src_id = tile_set->get_source_id(i);
                Dictionary src_info;
                src_info["source_id"] = static_cast<int64_t>(src_id);
                godot::Ref<godot::TileSetSource> src = tile_set->get_source(src_id);
                src_info["type"] = src.is_valid() ? src->get_class() : String("unknown");
                sources.append(src_info);
            }
            ts_info["sources"] = sources;
        } else {
            ts_info["source_count"] = 0;
            ts_info["sources"] = godot::Array();
        }
        data["tile_set"] = ts_info;

        // Used cells
        godot::TypedArray<godot::Vector2i> used_cells = tilemap->get_used_cells();
        data["used_cells_count"] = static_cast<int64_t>(used_cells.size());

        godot::Array used_cells_json;
        for (int64_t i = 0; i < used_cells.size(); i++) {
            godot::Vector2i cell = used_cells[i];
            used_cells_json.append(godot::Array::make(static_cast<int64_t>(cell.x), static_cast<int64_t>(cell.y)));
        }
        data["used_cells"] = used_cells_json;

        // Used rect
        godot::Rect2i rect = tilemap->get_used_rect();
        Dictionary rect_data;
        Dictionary pos_data;
        pos_data["x"] = static_cast<int64_t>(rect.position.x);
        pos_data["y"] = static_cast<int64_t>(rect.position.y);
        rect_data["position"] = pos_data;
        Dictionary size_data;
        size_data["w"] = static_cast<int64_t>(rect.size.x);
        size_data["h"] = static_cast<int64_t>(rect.size.y);
        rect_data["size"] = size_data;
        data["used_rect"] = rect_data;

        // Other properties
        data["collision_enabled"] = tilemap->is_collision_enabled();
        data["navigation_enabled"] = tilemap->is_navigation_enabled();
        data["y_sort_origin"] = static_cast<int64_t>(tilemap->get_y_sort_origin());
        data["rendering_quadrant_size"] = static_cast<int64_t>(tilemap->get_rendering_quadrant_size());

        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

