// 鈹€鈹€ Signal tools 鈹€鈹€
GODOT_MCP_TOOL(ConnectSignalTool,           "connect_signal",           "node_tools/signal", false, true, true, false)
GODOT_MCP_TOOL(DisconnectSignalTool,        "disconnect_signal",        "node_tools/signal", false, true, true, false)
GODOT_MCP_TOOL(ListSignalsTool,             "list_signals",             "node_tools/signal", false, true, true, false)
GODOT_MCP_TOOL(GetSignalConnectionsTool,    "get_signal_connections",   "node_tools/signal", false, true, true, false)

// 鈹€鈹€ Group tools 鈹€鈹€
GODOT_MCP_TOOL(AddToGroupTool,              "add_to_group",             "node_tools/group", false, true, true, false)
GODOT_MCP_TOOL(RemoveFromGroupTool,         "remove_from_group",        "node_tools/group", false, true, true, false)
GODOT_MCP_TOOL(GetNodeGroupsTool,           "get_node_groups",          "node_tools/group", false, true, true, false)
GODOT_MCP_TOOL(GetNodesInGroupTool,         "get_nodes_in_group",       "node_tools/group", false, true, true, false)

// 鈹€鈹€ General resource tools 鈹€鈹€
GODOT_MCP_TOOL(SaveResourceTool,            "save_resource",            "node_tools/general", false, true, true, false)
GODOT_MCP_TOOL(LoadResourceTool,            "load_resource",            "node_tools/general", false, true, true, false)
GODOT_MCP_TOOL(NewResourceTool,             "new_resource",             "node_tools/general", false, true, true, false)
GODOT_MCP_TOOL(DuplicateResourceTool,       "duplicate_resource",       "node_tools/general", false, true, true, false)
GODOT_MCP_TOOL(ClearResourceTool,           "clear_resource",           "node_tools/general", false, true, true, false)
GODOT_MCP_TOOL(GetResourceInfoTool,         "get_resource_info",        "node_tools/general", false, true, true, false)

// 鈹€鈹€ Scene tree tools 鈹€鈹€
GODOT_MCP_TOOL(AddNodeTool,                 "add_node",                 "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(DeleteNodeTool,              "delete_node",              "editor_tools/scene_tree", false, true, false, true)
GODOT_MCP_TOOL(RenameNodeTool,              "rename_node",              "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(MoveNodeTool,                "move_node",                "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(DuplicateNodeTool,           "duplicate_node",           "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(CopyNodeTool,                "copy_node",                "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(PasteNodeTool,               "paste_node",               "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(CutNodeTool,                 "cut_node",                 "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(GetSceneTreeTool,            "get_scene_tree",           "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(SaveSceneTool,               "save_scene",               "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(NewSceneTool,                "new_scene",                "editor_tools/scene_tree", false, false, false, false)
GODOT_MCP_TOOL(ChangeNodeTypeTool,          "change_node_type",         "editor_tools/scene_tree", false, true, true, true)
GODOT_MCP_TOOL(AttachScriptTool,            "attach_script",            "editor_tools/scene_tree", false, true, true, false)
GODOT_MCP_TOOL(DetachScriptTool,            "detach_script",            "editor_tools/scene_tree", false, true, true, false)
GODOT_MCP_TOOL(ReparentNodeTool,            "reparent_node",            "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(ReparentToNewNodeTool,       "reparent_to_new_node",     "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(GroupSelectedNodesTool,      "group_selected_nodes",     "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(MakeLocalTool,               "make_local",               "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(SaveBranchAsSceneTool,       "save_branch_as_scene",     "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(InstanceChildSceneTool,      "instance_child_scene",     "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(SetUniqueNameTool,           "set_unique_name",          "editor_tools/scene_tree", false, true, true, false)
GODOT_MCP_TOOL(ToggleEditableChildrenTool,  "toggle_editable_children", "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(ToggleEditGroupTool,         "toggle_edit_group",        "editor_tools/scene_tree", false, true, false, false)
GODOT_MCP_TOOL(TogglePlaceholderTool,       "toggle_placeholder",       "editor_tools/scene_tree", false, true, false, false)

// 鈹€鈹€ Animation tools 鈹€鈹€
GODOT_MCP_TOOL(CreateAnimationPlayerTool,   "create_animation_player",  "editor_tools/animation", false, true, false, false)
GODOT_MCP_TOOL(CreateAnimationClipTool,     "create_animation_clip",    "editor_tools/animation", false, true, false, false)
GODOT_MCP_TOOL(AddAnimationTrackTool,       "add_animation_track",      "editor_tools/animation", false, true, false, false)
GODOT_MCP_TOOL(SetKeyframeTool,             "set_keyframe",             "editor_tools/animation", false, true, false, false)
GODOT_MCP_TOOL(GetAnimationInfoTool,        "get_animation_info",       "editor_tools/animation", false, true, false, false)
GODOT_MCP_TOOL(CreateAnimationTreeTool,     "create_animation_tree",    "editor_tools/animation", false, true, false, false)
GODOT_MCP_TOOL(GetAnimationTreeInfoTool,    "get_animation_tree_info",  "editor_tools/animation", false, true, false, false)
GODOT_MCP_TOOL(AddAnimationNodeTool,        "add_animation_node",       "editor_tools/animation", false, true, false, false)
GODOT_MCP_TOOL(AddTransitionTool,           "add_transition",           "editor_tools/animation", false, true, false, false)
GODOT_MCP_TOOL(SetTransitionConditionTool,  "set_transition_condition", "editor_tools/animation", false, true, false, false)

// 鈹€鈹€ Control tools 鈹€鈹€
GODOT_MCP_TOOL(CreateControlTool,           "create_control",           "editor_tools/control", false, true, false, false)
GODOT_MCP_TOOL(CreateStyleBoxTool,          "create_stylebox",          "editor_tools/control", false, true, false, false)
GODOT_MCP_TOOL(SetLayoutPresetTool,         "set_layout_preset",        "editor_tools/control", false, true, true, false)
GODOT_MCP_TOOL(SetThemeOverrideTool,        "set_theme_override",       "editor_tools/control", false, true, true, false)

// 鈹€鈹€ Collision tools 鈹€鈹€
GODOT_MCP_TOOL(CreateCollisionShapeTool,    "create_collision_shape",   "editor_tools/collision", false, true, false, false)

// 鈹€鈹€ Export tools 鈹€鈹€
GODOT_MCP_TOOL(ListExportPresetsTool,       "list_export_presets",      "editor_tools/export", false, false, false, false)
GODOT_MCP_TOOL(ExportProjectTool,           "export_project",           "editor_tools/export", false, false, false, true)
GODOT_MCP_TOOL(ValidateExportPresetsTool,   "validate_export_presets",  "editor_tools/export", false, false, false, false)
GODOT_MCP_TOOL(GetExportPlatformsTool,      "get_export_platforms",     "editor_tools/export", false, false, false, false)

// 鈹€鈹€ Filesystem tools 鈹€鈹€
GODOT_MCP_TOOL(CreateTool,                  "create",                   "editor_tools/filesystem", false, false, false, false)
GODOT_MCP_TOOL(CreateDirectoryTool,         "create_directory",         "editor_tools/filesystem", false, false, false, false)
GODOT_MCP_TOOL(CreateSceneTool,             "create_scene",             "editor_tools/filesystem", false, false, false, false)
GODOT_MCP_TOOL(CreateResourceTool,          "create_resource",          "editor_tools/filesystem", false, false, false, false)
GODOT_MCP_TOOL(CreateGdshaderTool,          "create_gdshader",          "editor_tools/filesystem", false, false, false, false)
GODOT_MCP_TOOL(DeleteFileTool,              "delete_file",              "editor_tools/filesystem", false, false, false, true)
GODOT_MCP_TOOL(MoveFileTool,                "move_file",                "editor_tools/filesystem", false, false, false, true)
GODOT_MCP_TOOL(CopyFileTool,                "copy_file",                "editor_tools/filesystem", false, false, false, false)
GODOT_MCP_TOOL(OpenFileTool,                "open_file",                "editor_tools/filesystem", false, false, false, false)
GODOT_MCP_TOOL(ListDirectoryTool,           "list_directory",           "editor_tools/filesystem", false, false, false, false)
GODOT_MCP_TOOL(SearchFilesTool,             "search_files",             "editor_tools/filesystem", false, false, false, false)
GODOT_MCP_TOOL(SaveResourceAsTool,          "save_resource_as",         "editor_tools/filesystem", false, false, false, false)

// 鈹€鈹€ Input map tools 鈹€鈹€
GODOT_MCP_TOOL(InputListActionsTool,        "input_list_actions",       "editor_tools/inputmap", false, false, false, false)
GODOT_MCP_TOOL(AddInputActionTool,          "add_input_action",         "editor_tools/inputmap", false, false, false, false)
GODOT_MCP_TOOL(RemoveInputActionTool,       "remove_input_action",      "editor_tools/inputmap", false, false, false, false)
GODOT_MCP_TOOL(AddInputEventBindingTool,    "add_input_event_binding",  "editor_tools/inputmap", false, false, false, false)

// 鈹€鈹€ Plugin tools 鈹€鈹€
GODOT_MCP_TOOL(ListPluginsTool,             "list_plugins",             "editor_tools/plugin", false, false, false, false)
GODOT_MCP_TOOL(EnablePluginTool,            "enable_plugin",            "editor_tools/plugin", false, false, false, false)
GODOT_MCP_TOOL(DisablePluginTool,           "disable_plugin",           "editor_tools/plugin", false, false, false, false)

// 鈹€鈹€ Scaffold tools 鈹€鈹€
GODOT_MCP_TOOL(CreateProjectTool,           "create_project",           "editor_tools/scaffold", false, false, false, false)

// 鈹€鈹€ Script tools 鈹€鈹€
GODOT_MCP_TOOL(ReadGdScriptTool,            "read_gd_script",           "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(WriteGdScriptTool,           "write_gd_script",          "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(PatchGdScriptTool,           "patch_gd_script",          "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(ValidateGdScriptTool,        "validate_gd_script",       "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(ListGdScriptsTool,           "list_gd_scripts",          "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(GrepScriptsTool,             "grep_scripts",             "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(GlobScriptsTool,             "glob_scripts",             "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(ReadCsharpScriptTool,        "read_csharp_script",       "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(WriteCsharpScriptTool,       "write_csharp_script",      "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(PatchCsharpScriptTool,       "patch_csharp_script",      "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(ValidateCsharpScriptTool,    "validate_csharp_script",   "editor_tools/scripts", false, false, false, false)
GODOT_MCP_TOOL(ListCsharpScriptsTool,       "list_csharp_scripts",      "editor_tools/scripts", false, false, false, false)

// 鈹€鈹€ Settings tools 鈹€鈹€
GODOT_MCP_TOOL(GetSettingTool,              "get_setting",              "editor_tools/settings", false, false, false, false)
GODOT_MCP_TOOL(SetSettingTool,              "set_setting",              "editor_tools/settings", false, false, false, false)
GODOT_MCP_TOOL(ResetSettingTool,            "reset_setting",            "editor_tools/settings", false, false, false, false)
GODOT_MCP_TOOL(ListSettingsTool,            "list_settings",            "editor_tools/settings", true, false, false, false)

// 鈹€鈹€ Shader tools 鈹€鈹€
GODOT_MCP_TOOL(CreateShaderTool,            "create_shader",            "editor_tools/shader", false, false, false, false)
GODOT_MCP_TOOL(ReadShaderTool,              "read_shader",              "editor_tools/shader", false, false, false, false)
GODOT_MCP_TOOL(ApplyShaderPresetTool,       "apply_shader_preset",      "editor_tools/shader", false, true, false, false)
GODOT_MCP_TOOL(GetShaderUniformsTool,       "get_shader_uniforms",      "editor_tools/shader", false, false, false, false)
GODOT_MCP_TOOL(SetShaderUniformTool,        "set_shader_uniform",       "editor_tools/shader", false, true, false, false)

// 鈹€鈹€ Audio tools 鈹€鈹€
GODOT_MCP_TOOL(CreateAudioPlayerTool,       "create_audio_player",      "editor_tools/audio", false, true, false, false)
GODOT_MCP_TOOL(SetAudioStreamTool,          "set_audio_stream",         "editor_tools/audio", false, true, false, false)
GODOT_MCP_TOOL(ListAudioBusesTool,          "list_audio_buses",         "editor_tools/audio", false, false, false, false)

// 鈹€鈹€ Navigation tools 鈹€鈹€
GODOT_MCP_TOOL(CreateNavigationRegionTool,  "create_navigation_region", "editor_tools/navigation", false, true, false, false)
GODOT_MCP_TOOL(CreateNavigationAgentTool,   "create_navigation_agent",  "editor_tools/navigation", false, true, false, false)
GODOT_MCP_TOOL(BakeNavigationMeshTool,      "bake_navigation_mesh",     "editor_tools/navigation", false, true, false, false)

// 鈹€鈹€ 3D Scene tools 鈹€鈹€
GODOT_MCP_TOOL(CreateMeshInstance3DTool,    "create_mesh_instance_3d",  "editor_tools/3d_scene", false, true, false, false)
GODOT_MCP_TOOL(CreateLight3DTool,           "create_light_3d",          "editor_tools/3d_scene", false, true, false, false)
GODOT_MCP_TOOL(SetWorldEnvironmentTool,     "set_world_environment",    "editor_tools/3d_scene", false, true, false, false)

// 鈹€鈹€ TileMap tools 鈹€鈹€
GODOT_MCP_TOOL(GetTileMapInfoTool,          "get_tilemap_info",         "editor_tools/tilemap", false, true, true, false)
GODOT_MCP_TOOL(SetTileMapCellsTool,         "set_tilemap_cells",        "editor_tools/tilemap", false, true, true, false)
GODOT_MCP_TOOL(EraseTileMapCellsTool,       "erase_tilemap_cells",      "editor_tools/tilemap", false, true, true, false)

// 鈹€鈹€ Visualizer tools 鈹€鈹€
GODOT_MCP_TOOL(GetProjectGraphTool,         "get_project_graph",        "editor_tools/visualizer", false, false, false, false)

// 鈹€鈹€ Workspace / Debugger tools 鈹€鈹€
GODOT_MCP_TOOL(CaptureViewportTool,         "capture_viewport",         "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(CaptureGameViewportTool,     "capture_game_viewport",    "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(ClearConsoleTool,            "clear_console",            "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetConsoleOutputTool,        "get_console_output",       "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetConsoleErrorsTool,        "get_console_errors",       "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetConsoleWarningsTool,      "get_console_warnings",     "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetDebuggerStateTool,        "get_debugger_state",       "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetDebuggerStatusTool,       "get_debugger_status",      "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetDebuggerErrorsTool,       "get_debugger_errors",      "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetFpsTool,                  "get_fps",                  "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetMemoryUsageTool,          "get_memory_usage",         "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetObjectCountTool,          "get_object_count",         "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetPerformanceMonitorsTool,  "get_performance_monitors", "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetPhysicsStatsTool,         "get_physics_stats",        "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetRenderStatsTool,          "get_render_stats",         "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetStackTraceTool,           "get_stack_trace",          "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(GetLocalsTool,               "get_locals",               "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(DebuggerBreakTool,           "debugger_break",           "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(DebuggerContinueTool,        "debugger_continue",        "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(DebuggerControlTool,         "debugger_control",         "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(DebuggerStepIntoTool,        "debugger_step_into",       "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(DebuggerStepOutTool,         "debugger_step_out",        "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(DebuggerStepOverTool,        "debugger_step_over",       "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(ListBreakpointsTool,         "list_breakpoints",         "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(SetBreakpointTool,           "set_breakpoint",           "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(RemoveBreakpointTool,        "remove_breakpoint",        "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(SetWorkspaceTool,            "set_workspace",            "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(SetWorkspace2DTool,          "set_workspace_2d",         "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(SetWorkspace3DTool,          "set_workspace_3d",         "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(SetWorkspaceScriptTool,      "set_workspace_script",     "editor_tools/workspace", false, false, false, false)
GODOT_MCP_TOOL(SetWorkspaceAssetLibTool,    "set_workspace_assetlib",   "editor_tools/workspace", false, false, false, false)

// 鈹€鈹€ Runtime bridge tools 鈹€鈹€
GODOT_MCP_TOOL(GetGameSceneTreeTool,        "get_game_scene_tree",      "runtime_tools/bridge", false, false, false, false)
GODOT_MCP_TOOL(GetGameNodePropertyTool,     "get_game_node_property",   "runtime_tools/bridge", false, false, false, false)
GODOT_MCP_TOOL(SetGameNodePropertyTool,     "set_game_node_property",   "runtime_tools/bridge", false, false, false, true)
GODOT_MCP_TOOL(CallMethodInGameTool,        "call_method_in_game",      "runtime_tools/bridge", false, false, false, false)
GODOT_MCP_TOOL(CaptureGameScreenshotTool,   "capture_game_screenshot",  "runtime_tools/bridge", false, false, false, false)
GODOT_MCP_TOOL(SimulateGameInputTool,       "simulate_game_input",      "runtime_tools/bridge", false, false, false, false)

// 鈹€鈹€ Runtime lifecycle tools 鈹€鈹€
GODOT_MCP_TOOL(RunProjectTool,              "run_project",              "runtime_tools/lifecycle", false, false, false, false)
GODOT_MCP_TOOL(RunCurrentSceneTool,         "run_current_scene",        "runtime_tools/lifecycle", false, false, false, false)
GODOT_MCP_TOOL(RunSpecificSceneTool,        "run_specific_scene",       "runtime_tools/lifecycle", false, false, false, false)
GODOT_MCP_TOOL(StopProjectTool,             "stop_project",             "runtime_tools/lifecycle", false, false, false, false)
GODOT_MCP_TOOL(PauseProjectTool,            "pause_project",            "runtime_tools/lifecycle", false, false, false, false)
GODOT_MCP_TOOL(SetMovieMakerTool,           "set_movie_maker",          "runtime_tools/lifecycle", false, false, false, false)
