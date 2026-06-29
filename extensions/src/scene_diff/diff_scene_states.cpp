#include "diff_scene_states.hpp"
#include "scene_diff.hpp"

#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/memory.hpp>

using namespace godot;

namespace godot_mcp {

using scene_diff::SceneDiff;

Dictionary DiffSceneStatesTool::build_input_schema() const {
    return SchemaBuilder()
        .prop("scene_path_a", "string", "Path to the first .tscn file (e.g. res://scenes/scene_a.tscn)")
        .prop("scene_path_b", "string", "Path to the second .tscn file (e.g. res://scenes/scene_b.tscn)")
        .required(Array::make("scene_path_a", "scene_path_b"))
        .build();
}

Dictionary DiffSceneStatesTool::execute_impl(const ToolContext &ctx) {
    String path_a = args_string(ctx.args, "scene_path_a");
    String path_b = args_string(ctx.args, "scene_path_b");

    if (path_a.is_empty() || path_b.is_empty()) {
        return ToolResult::err("MISSING_PARAM", "Both scene_path_a and scene_path_b are required");
    }

    Ref<PackedScene> scene_a = ResourceLoader::get_singleton()->load(path_a, "PackedScene");
    if (scene_a.is_null()) {
        return ToolResult::err("LOAD_FAILED", "Failed to load scene: " + path_a);
    }

    Ref<PackedScene> scene_b = ResourceLoader::get_singleton()->load(path_b, "PackedScene");
    if (scene_b.is_null()) {
        return ToolResult::err("LOAD_FAILED", "Failed to load scene: " + path_b);
    }

    Node *root_a = scene_a->instantiate();
    if (!root_a) {
        return ToolResult::err("INSTANTIATE_FAILED", "Failed to instantiate: " + path_a);
    }

    Node *root_b = scene_b->instantiate();
    if (!root_b) {
        memdelete(root_a);
        return ToolResult::err("INSTANTIATE_FAILED", "Failed to instantiate: " + path_b);
    }

    scene_diff::DiffResult diff = SceneDiff::compute(root_a, root_b);

    memdelete(root_a);
    memdelete(root_b);

    return ToolResult::ok(diff.to_dict());
}

} // namespace godot_mcp
