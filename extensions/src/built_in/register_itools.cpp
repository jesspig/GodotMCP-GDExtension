#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244) // int64_t→int from Godot API calls in tool headers
#endif

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

#include "server/registry/handler_registry.hpp"
#include "built_in/cmd_utils/dispatch_map.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"

// ── Meta tools ──
#include "built_in/tools/meta/get_info.hpp"
#include "built_in/tools/meta/get_tools.hpp"
#include "built_in/tools/meta/get_categories.hpp"
#include "built_in/tools/meta/find_tool.hpp"
#include "built_in/tools/meta/call_tool.hpp"
#include "built_in/tools/meta/undo.hpp"
#include "built_in/tools/meta/redo.hpp"
#include "built_in/tools/meta/get_undo_history.hpp"
#include "built_in/tools/meta/execute_workflow.hpp"
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
#include "built_in/tools/editor_tools/animation/create_animation_tree.hpp"
#include "built_in/tools/editor_tools/animation/get_animation_tree_info.hpp"
#include "built_in/tools/editor_tools/animation/add_animation_node.hpp"
#include "built_in/tools/editor_tools/animation/add_transition.hpp"
#include "built_in/tools/editor_tools/animation/set_transition_condition.hpp"

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
#include "built_in/tools/editor_tools/docs/get_class_list.hpp"
#include "built_in/tools/editor_tools/docs/get_inheritance_chain.hpp"
#include "built_in/tools/editor_tools/docs/get_property_doc.hpp"
#include "built_in/tools/editor_tools/docs/get_method_doc.hpp"
#include "built_in/tools/editor_tools/docs/get_enum_doc.hpp"

// ── Export tools ──
#include "built_in/tools/editor_tools/export/list_export_presets.hpp"
#include "built_in/tools/editor_tools/export/export_project.hpp"
#include "built_in/tools/editor_tools/export/validate_export_presets.hpp"
#include "built_in/tools/editor_tools/export/get_export_platforms.hpp"

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
#include "built_in/tools/editor_tools/inputmap/add_input_action.hpp"
#include "built_in/tools/editor_tools/inputmap/remove_input_action.hpp"
#include "built_in/tools/editor_tools/inputmap/add_input_event_binding.hpp"

// ── Plugin tools ──
#include "built_in/tools/editor_tools/plugin/list_plugins.hpp"
#include "built_in/tools/editor_tools/plugin/set_plugin_enabled.hpp"

// ── Scaffold tools ──
#include "built_in/tools/editor_tools/scaffold/create_project.hpp"

// ── Script tools ──
#include "built_in/tools/editor_tools/scripts/read_script.hpp"
#include "built_in/tools/editor_tools/scripts/write_script.hpp"
#include "built_in/tools/editor_tools/scripts/patch_script.hpp"
#include "built_in/tools/editor_tools/scripts/validate_script.hpp"
#include "built_in/tools/editor_tools/scripts/list_scripts.hpp"
#include "built_in/tools/editor_tools/scripts/grep_scripts.hpp"
#include "built_in/tools/editor_tools/scripts/glob_scripts.hpp"
#include "built_in/tools/editor_tools/scripts/run_editor_script.hpp"

// ── Settings tools ──
#include "built_in/tools/editor_tools/settings/get_setting.hpp"
#include "built_in/tools/editor_tools/settings/set_setting.hpp"
#include "built_in/tools/editor_tools/settings/reset_setting.hpp"
#include "built_in/tools/editor_tools/settings/list_settings.hpp"

// ── Shader tools ──
#include "built_in/tools/editor_tools/shader/create_shader.hpp"
#include "built_in/tools/editor_tools/shader/read_shader.hpp"
#include "built_in/tools/editor_tools/shader/apply_shader_preset.hpp"
#include "built_in/tools/editor_tools/shader/get_shader_uniforms.hpp"
#include "built_in/tools/editor_tools/shader/set_shader_uniform.hpp"

// ── Audio tools ──
#include "built_in/tools/editor_tools/audio/create_audio_player.hpp"
#include "built_in/tools/editor_tools/audio/set_audio_stream.hpp"
#include "built_in/tools/editor_tools/audio/list_audio_buses.hpp"

// ── Navigation tools ──
#include "built_in/tools/editor_tools/navigation/create_navigation_region.hpp"
#include "built_in/tools/editor_tools/navigation/create_navigation_agent.hpp"
#include "built_in/tools/editor_tools/navigation/bake_navigation_mesh.hpp"

// ── 3D Scene tools ──
#include "built_in/tools/editor_tools/3d_scene/create_mesh_instance_3d.hpp"
#include "built_in/tools/editor_tools/3d_scene/create_light_3d.hpp"
#include "built_in/tools/editor_tools/3d_scene/set_world_environment.hpp"

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
#include "built_in/tools/editor_tools/workspace/get_debugger_state.hpp"
#include "built_in/tools/editor_tools/workspace/get_performance_monitors.hpp"
#include "built_in/tools/editor_tools/workspace/get_stack_trace.hpp"
#include "built_in/tools/editor_tools/workspace/get_locals.hpp"
#include "built_in/tools/editor_tools/workspace/debugger_control.hpp"
#include "built_in/tools/editor_tools/workspace/list_breakpoints.hpp"
#include "built_in/tools/editor_tools/workspace/set_breakpoint.hpp"
#include "built_in/tools/editor_tools/workspace/remove_breakpoint.hpp"
#include "built_in/tools/editor_tools/workspace/set_workspace.hpp"

// ── Runtime bridge tools ──
#include "built_in/tools/runtime_tools/bridge/wait_for_bridge.hpp"
#include "built_in/tools/runtime_tools/bridge/list_game_instances.hpp"
#include "built_in/tools/runtime_tools/bridge/get_game_scene_tree.hpp"
#include "built_in/tools/runtime_tools/bridge/get_game_node_property.hpp"
#include "built_in/tools/runtime_tools/bridge/set_game_node_property.hpp"
#include "built_in/tools/runtime_tools/bridge/call_method_in_game.hpp"
#include "built_in/tools/runtime_tools/bridge/capture_game_screenshot.hpp"
#include "built_in/tools/runtime_tools/bridge/simulate_game_input.hpp"

// ── Shadow Scene tools ──
#include "built_in/tools/editor_tools/scene/stage_change.hpp"
#include "built_in/tools/editor_tools/scene/preview_change.hpp"
#include "built_in/tools/editor_tools/scene/apply_changes.hpp"
#include "built_in/tools/editor_tools/scene/discard_changes.hpp"

// ── Diff Scene States tool ──
#include "scene_diff/diff_scene_states.hpp"

// ── Recording / Replay tools ──
#include "replay/replay_tools.hpp"

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

// is_destructive_val �?the tool's virtual method overrides (name(), category(), etc.) are authoritative.
#define GODOT_MCP_TOOL(cls, is_destructive_val) \
    { \
        auto tool = std::make_unique<cls>(); \
        tool->set_is_destructive(is_destructive_val); \
        reg.register_tool(std::move(tool)); \
    }

void register_itools(HandlerRegistry &reg) {
    #include "built_in/tools/register/register_meta.hpp"
    #include "built_in/tools/register/register_existing.hpp"
    #include "built_in/tools/register/register_fallback.hpp"
    #include "built_in/tools/register/register_docs.hpp"
}

} // namespace godot_mcp

#ifdef _MSC_VER
#pragma warning(pop)
#endif
