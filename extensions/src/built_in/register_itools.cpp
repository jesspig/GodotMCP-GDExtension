#include "server/registry/handler_registry.hpp"

// ── Meta tools ──
#include "built_in/tools/meta/get_info.hpp"
#include "built_in/tools/meta/get_tools.hpp"
#include "built_in/tools/meta/get_categories.hpp"
#include "built_in/tools/meta/get_tool_detail.hpp"
#include "built_in/tools/meta/find_tool.hpp"
#include "built_in/tools/meta/call_tool.hpp"

// ── Signal tools ──
#include "built_in/tools/signal/connect_signal.hpp"
#include "built_in/tools/signal/disconnect_signal.hpp"
#include "built_in/tools/signal/list_signals.hpp"
#include "built_in/tools/signal/get_signal_connections.hpp"

// ── Group tools ──
#include "built_in/tools/group/add_to_group.hpp"
#include "built_in/tools/group/remove_from_group.hpp"
#include "built_in/tools/group/get_node_groups.hpp"
#include "built_in/tools/group/get_nodes_in_group.hpp"

// ── General resource tools ──
#include "built_in/tools/node_tools/general/save_resource.hpp"
#include "built_in/tools/node_tools/general/load_resource.hpp"
#include "built_in/tools/node_tools/general/new_resource.hpp"
#include "built_in/tools/node_tools/general/duplicate_resource.hpp"
#include "built_in/tools/node_tools/general/clear_resource.hpp"
#include "built_in/tools/node_tools/general/get_resource_info.hpp"

// ── Scene tree tools ──
#include "built_in/tools/editor_tools/scene_tree/add_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/delete_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/rename_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/move_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/duplicate_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/copy_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/paste_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/cut_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/get_scene_tree.hpp"
#include "built_in/tools/editor_tools/scene_tree/save_scene.hpp"
#include "built_in/tools/editor_tools/scene_tree/new_scene.hpp"
#include "built_in/tools/editor_tools/scene_tree/change_node_type.hpp"
#include "built_in/tools/editor_tools/scene_tree/attach_script.hpp"
#include "built_in/tools/editor_tools/scene_tree/detach_script.hpp"
#include "built_in/tools/editor_tools/scene_tree/reparent_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/reparent_to_new_node.hpp"
#include "built_in/tools/editor_tools/scene_tree/group_selected_nodes.hpp"
#include "built_in/tools/editor_tools/scene_tree/make_local.hpp"
#include "built_in/tools/editor_tools/scene_tree/save_branch_as_scene.hpp"
#include "built_in/tools/editor_tools/scene_tree/instance_child_scene.hpp"
#include "built_in/tools/editor_tools/scene_tree/set_unique_name.hpp"
#include "built_in/tools/editor_tools/scene_tree/toggle_editable_children.hpp"
#include "built_in/tools/editor_tools/scene_tree/toggle_edit_group.hpp"
#include "built_in/tools/editor_tools/scene_tree/toggle_placeholder.hpp"

// ── Animation tools ──
#include "built_in/tools/editor_tools/animation/create_animation_player.hpp"
#include "built_in/tools/editor_tools/animation/create_animation_clip.hpp"
#include "built_in/tools/editor_tools/animation/add_animation_track.hpp"
#include "built_in/tools/editor_tools/animation/set_keyframe.hpp"
#include "built_in/tools/editor_tools/animation/get_animation_info.hpp"

// ── Control tools ──
#include "built_in/tools/editor_tools/control/create_control.hpp"
#include "built_in/tools/editor_tools/control/create_stylebox.hpp"
#include "built_in/tools/editor_tools/control/set_layout_preset.hpp"
#include "built_in/tools/editor_tools/control/set_theme_override.hpp"

// ── Collision tools ──
#include "built_in/tools/editor_tools/collision/create_collision_shape.hpp"

// ── Docs tools ──
#include "built_in/tools/editor_tools/docs/search_docs.hpp"
#include "built_in/tools/editor_tools/docs/get_class_info.hpp"
#include "built_in/tools/editor_tools/docs/get_best_practices.hpp"

// ── Export tools ──
#include "built_in/tools/editor_tools/export/list_export_presets.hpp"
#include "built_in/tools/editor_tools/export/export_project.hpp"

// ── Filesystem tools ──
#include "built_in/tools/editor_tools/filesystem/create.hpp"
#include "built_in/tools/editor_tools/filesystem/create_directory.hpp"
#include "built_in/tools/editor_tools/filesystem/create_scene.hpp"
#include "built_in/tools/editor_tools/filesystem/create_resource.hpp"
#include "built_in/tools/editor_tools/filesystem/create_gdshader.hpp"
#include "built_in/tools/editor_tools/filesystem/delete_file.hpp"
#include "built_in/tools/editor_tools/filesystem/move_file.hpp"
#include "built_in/tools/editor_tools/filesystem/copy_file.hpp"
#include "built_in/tools/editor_tools/filesystem/open_file.hpp"
#include "built_in/tools/editor_tools/filesystem/list_directory.hpp"
#include "built_in/tools/editor_tools/filesystem/search_files.hpp"
#include "built_in/tools/editor_tools/filesystem/save_resource_as.hpp"

// ── Input map tools ──
#include "built_in/tools/editor_tools/inputmap/input_list_actions.hpp"

// ── Plugin tools ──
#include "built_in/tools/editor_tools/plugin/list_plugins.hpp"
#include "built_in/tools/editor_tools/plugin/enable_plugin.hpp"
#include "built_in/tools/editor_tools/plugin/disable_plugin.hpp"

// ── Scaffold tools ──
#include "built_in/tools/editor_tools/scaffold/create_project.hpp"

// ── Script tools ──
#include "built_in/tools/editor_tools/scripts/read_gd_script.hpp"
#include "built_in/tools/editor_tools/scripts/write_gd_script.hpp"
#include "built_in/tools/editor_tools/scripts/patch_gd_script.hpp"
#include "built_in/tools/editor_tools/scripts/validate_gd_script.hpp"
#include "built_in/tools/editor_tools/scripts/list_gd_scripts.hpp"
#include "built_in/tools/editor_tools/scripts/grep_scripts.hpp"
#include "built_in/tools/editor_tools/scripts/glob_scripts.hpp"
#include "built_in/tools/editor_tools/scripts/read_csharp_script.hpp"
#include "built_in/tools/editor_tools/scripts/write_csharp_script.hpp"
#include "built_in/tools/editor_tools/scripts/patch_csharp_script.hpp"
#include "built_in/tools/editor_tools/scripts/validate_csharp_script.hpp"
#include "built_in/tools/editor_tools/scripts/list_csharp_scripts.hpp"

// ── Settings tools ──
#include "built_in/tools/editor_tools/settings/get_setting.hpp"
#include "built_in/tools/editor_tools/settings/set_setting.hpp"
#include "built_in/tools/editor_tools/settings/reset_setting.hpp"
#include "built_in/tools/editor_tools/settings/list_settings.hpp"

// ── Shader tools ──
#include "built_in/tools/editor_tools/shader/create_shader.hpp"
#include "built_in/tools/editor_tools/shader/read_shader.hpp"
#include "built_in/tools/editor_tools/shader/apply_shader_preset.hpp"

// ── TileMap tools ──
#include "built_in/tools/editor_tools/tilemap/get_tilemap_info.hpp"
#include "built_in/tools/editor_tools/tilemap/set_tilemap_cells.hpp"
#include "built_in/tools/editor_tools/tilemap/erase_tilemap_cells.hpp"

// ── Visualizer tools ──
#include "built_in/tools/editor_tools/visualizer/get_project_graph.hpp"

// ── Workspace / Debugger tools ──
#include "built_in/tools/editor_tools/workspace/capture_viewport.hpp"
#include "built_in/tools/editor_tools/workspace/capture_game_viewport.hpp"
#include "built_in/tools/editor_tools/workspace/clear_console.hpp"
#include "built_in/tools/editor_tools/workspace/get_console_output.hpp"
#include "built_in/tools/editor_tools/workspace/get_console_errors.hpp"
#include "built_in/tools/editor_tools/workspace/get_console_warnings.hpp"
#include "built_in/tools/editor_tools/workspace/get_debugger_state.hpp"
#include "built_in/tools/editor_tools/workspace/get_debugger_status.hpp"
#include "built_in/tools/editor_tools/workspace/get_debugger_errors.hpp"
#include "built_in/tools/editor_tools/workspace/get_fps.hpp"
#include "built_in/tools/editor_tools/workspace/get_memory_usage.hpp"
#include "built_in/tools/editor_tools/workspace/get_object_count.hpp"
#include "built_in/tools/editor_tools/workspace/get_performance_monitors.hpp"
#include "built_in/tools/editor_tools/workspace/get_physics_stats.hpp"
#include "built_in/tools/editor_tools/workspace/get_render_stats.hpp"
#include "built_in/tools/editor_tools/workspace/get_stack_trace.hpp"
#include "built_in/tools/editor_tools/workspace/get_locals.hpp"
#include "built_in/tools/editor_tools/workspace/debugger_break.hpp"
#include "built_in/tools/editor_tools/workspace/debugger_continue.hpp"
#include "built_in/tools/editor_tools/workspace/debugger_control.hpp"
#include "built_in/tools/editor_tools/workspace/debugger_step_into.hpp"
#include "built_in/tools/editor_tools/workspace/debugger_step_out.hpp"
#include "built_in/tools/editor_tools/workspace/debugger_step_over.hpp"
#include "built_in/tools/editor_tools/workspace/list_breakpoints.hpp"
#include "built_in/tools/editor_tools/workspace/set_breakpoint.hpp"
#include "built_in/tools/editor_tools/workspace/remove_breakpoint.hpp"
#include "built_in/tools/editor_tools/workspace/set_workspace.hpp"
#include "built_in/tools/editor_tools/workspace/set_workspace_2d.hpp"
#include "built_in/tools/editor_tools/workspace/set_workspace_3d.hpp"
#include "built_in/tools/editor_tools/workspace/set_workspace_script.hpp"
#include "built_in/tools/editor_tools/workspace/set_workspace_assetlib.hpp"

// ── Runtime bridge tools ──
#include "built_in/tools/runtime_tools/bridge/get_game_scene_tree.hpp"
#include "built_in/tools/runtime_tools/bridge/get_game_node_property.hpp"
#include "built_in/tools/runtime_tools/bridge/set_game_node_property.hpp"
#include "built_in/tools/runtime_tools/bridge/call_method_in_game.hpp"
#include "built_in/tools/runtime_tools/bridge/capture_game_screenshot.hpp"
#include "built_in/tools/runtime_tools/bridge/simulate_game_input.hpp"

// ── Runtime lifecycle tools ──
#include "built_in/tools/runtime_tools/lifecycle/run_project.hpp"
#include "built_in/tools/runtime_tools/lifecycle/run_current_scene.hpp"
#include "built_in/tools/runtime_tools/lifecycle/run_specific_scene.hpp"
#include "built_in/tools/runtime_tools/lifecycle/stop_project.hpp"
#include "built_in/tools/runtime_tools/lifecycle/pause_project.hpp"
#include "built_in/tools/runtime_tools/lifecycle/set_movie_maker.hpp"

// ── Layer 0 fallback tools ──
#include "built_in/tools/node_properties/fallback_tools.hpp"

using namespace godot;

namespace godot_mcp {

#define GODOT_MCP_TOOL(cls, name_str, cat, is_meta_val, need_scene_val, need_node_val) \
    { \
        auto tool = std::make_unique<cls>(); \
        reg.register_tool(std::move(tool)); \
    }

void register_itools(HandlerRegistry &reg) {
    #include "built_in/tools/register/register_meta.hpp"
    #include "built_in/tools/register/register_existing.hpp"
    #include "built_in/tools/register/register_fallback.hpp"
    #include "built_in/tools/register/register_docs.hpp"
}

} // namespace godot_mcp
