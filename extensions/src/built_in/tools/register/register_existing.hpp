// Signal tools
GODOT_MCP_TOOL(ConnectSignalTool, false)
GODOT_MCP_TOOL(DisconnectSignalTool, false)
GODOT_MCP_TOOL(ListSignalsTool, false)
GODOT_MCP_TOOL(GetSignalConnectionsTool, false)

// Group tools
GODOT_MCP_TOOL(AddToGroupTool, false)
GODOT_MCP_TOOL(RemoveFromGroupTool, false)
GODOT_MCP_TOOL(GetNodeGroupsTool, false)
GODOT_MCP_TOOL(GetNodesInGroupTool, false)

// General resource tools
GODOT_MCP_TOOL(SaveResourceTool, false)
GODOT_MCP_TOOL(LoadResourceTool, false)
GODOT_MCP_TOOL(NewResourceTool, false)
GODOT_MCP_TOOL(DuplicateResourceTool, false)
GODOT_MCP_TOOL(ClearResourceTool, false)
GODOT_MCP_TOOL(GetResourceInfoTool, false)

// Scene tree tools
GODOT_MCP_TOOL(AddNodeTool, false)
GODOT_MCP_TOOL(DeleteNodeTool, true)
GODOT_MCP_TOOL(RenameNodeTool, false)
GODOT_MCP_TOOL(MoveNodeTool, false)
GODOT_MCP_TOOL(DuplicateNodeTool, false)
GODOT_MCP_TOOL(CopyNodeTool, false)
GODOT_MCP_TOOL(PasteNodeTool, false)
GODOT_MCP_TOOL(CutNodeTool, false)
GODOT_MCP_TOOL(GetSceneTreeTool, false)
GODOT_MCP_TOOL(SaveSceneTool, true)
GODOT_MCP_TOOL(NewSceneTool, false)
GODOT_MCP_TOOL(ChangeNodeTypeTool, true)
GODOT_MCP_TOOL(AttachScriptTool, false)
GODOT_MCP_TOOL(DetachScriptTool, false)
GODOT_MCP_TOOL(ReparentNodeTool, false)
GODOT_MCP_TOOL(ReparentToNewNodeTool, false)
GODOT_MCP_TOOL(GroupSelectedNodesTool, false)
GODOT_MCP_TOOL(MakeLocalTool, false)
GODOT_MCP_TOOL(SaveBranchAsSceneTool, false)
GODOT_MCP_TOOL(InstanceChildSceneTool, false)
GODOT_MCP_TOOL(SetUniqueNameTool, false)
GODOT_MCP_TOOL(ToggleEditableChildrenTool, false)
GODOT_MCP_TOOL(ToggleEditGroupTool, false)
GODOT_MCP_TOOL(TogglePlaceholderTool, false)

// Animation tools
GODOT_MCP_TOOL(CreateAnimationPlayerTool, false)
GODOT_MCP_TOOL(CreateAnimationClipTool, false)
GODOT_MCP_TOOL(AddAnimationTrackTool, false)
GODOT_MCP_TOOL(SetKeyframeTool, false)
GODOT_MCP_TOOL(GetAnimationInfoTool, false)
GODOT_MCP_TOOL(CreateAnimationTreeTool, false)
GODOT_MCP_TOOL(GetAnimationTreeInfoTool, false)
GODOT_MCP_TOOL(AddAnimationNodeTool, false)
GODOT_MCP_TOOL(AddTransitionTool, false)
GODOT_MCP_TOOL(SetTransitionConditionTool, false)

// Control tools
GODOT_MCP_TOOL(CreateControlTool, false)
GODOT_MCP_TOOL(CreateStyleBoxTool, false)
GODOT_MCP_TOOL(SetLayoutPresetTool, false)
GODOT_MCP_TOOL(SetThemeOverrideTool, false)

// Collision tools
GODOT_MCP_TOOL(CreateCollisionShapeTool, false)

// Export tools
GODOT_MCP_TOOL(ListExportPresetsTool, false)
GODOT_MCP_TOOL(ExportProjectTool, true)
GODOT_MCP_TOOL(ValidateExportPresetsTool, false)
GODOT_MCP_TOOL(GetExportPlatformsTool, false)

// Filesystem tools
GODOT_MCP_TOOL(CreateTool, false)
GODOT_MCP_TOOL(CreateDirectoryTool, false)
GODOT_MCP_TOOL(CreateSceneTool, false)
GODOT_MCP_TOOL(CreateResourceTool, false)
GODOT_MCP_TOOL(CreateGdshaderTool, false)
GODOT_MCP_TOOL(DeleteFileTool, true)
GODOT_MCP_TOOL(MoveFileTool, true)
GODOT_MCP_TOOL(CopyFileTool, true)
GODOT_MCP_TOOL(OpenFileTool, false)
GODOT_MCP_TOOL(ListDirectoryTool, false)
GODOT_MCP_TOOL(SearchFilesTool, false)
GODOT_MCP_TOOL(SaveResourceAsTool, false)

// Input map tools
GODOT_MCP_TOOL(InputListActionsTool, false)
GODOT_MCP_TOOL(AddInputActionTool, false)
GODOT_MCP_TOOL(RemoveInputActionTool, false)
GODOT_MCP_TOOL(AddInputEventBindingTool, false)

// Plugin tools
GODOT_MCP_TOOL(ListPluginsTool, false)
GODOT_MCP_TOOL(EnablePluginTool, false)
GODOT_MCP_TOOL(DisablePluginTool, false)

// Scaffold tools
GODOT_MCP_TOOL(CreateProjectTool, false)

// Script tools
GODOT_MCP_TOOL(ReadGdScriptTool, false)
GODOT_MCP_TOOL(WriteGdScriptTool, true)
GODOT_MCP_TOOL(PatchGdScriptTool, true)
GODOT_MCP_TOOL(ValidateGdScriptTool, false)
GODOT_MCP_TOOL(ListGdScriptsTool, false)
GODOT_MCP_TOOL(GrepScriptsTool, false)
GODOT_MCP_TOOL(GlobScriptsTool, false)
GODOT_MCP_TOOL(ReadCsharpScriptTool, false)
GODOT_MCP_TOOL(WriteCsharpScriptTool, false)
GODOT_MCP_TOOL(PatchCsharpScriptTool, false)
GODOT_MCP_TOOL(ValidateCsharpScriptTool, false)
GODOT_MCP_TOOL(ListCsharpScriptsTool, false)

// Settings tools
GODOT_MCP_TOOL(GetSettingTool, false)
GODOT_MCP_TOOL(SetSettingTool, true)
GODOT_MCP_TOOL(ResetSettingTool, false)
GODOT_MCP_TOOL(ListSettingsTool, false)

// Shader tools
GODOT_MCP_TOOL(CreateShaderTool, false)
GODOT_MCP_TOOL(ReadShaderTool, false)
GODOT_MCP_TOOL(ApplyShaderPresetTool, false)
GODOT_MCP_TOOL(GetShaderUniformsTool, false)
GODOT_MCP_TOOL(SetShaderUniformTool, false)

// Audio tools
GODOT_MCP_TOOL(CreateAudioPlayerTool, false)
GODOT_MCP_TOOL(SetAudioStreamTool, false)
GODOT_MCP_TOOL(ListAudioBusesTool, false)

// Navigation tools
GODOT_MCP_TOOL(CreateNavigationRegionTool, false)
GODOT_MCP_TOOL(CreateNavigationAgentTool, false)
GODOT_MCP_TOOL(BakeNavigationMeshTool, false)

// 3D Scene tools
GODOT_MCP_TOOL(CreateMeshInstance3DTool, false)
GODOT_MCP_TOOL(CreateLight3DTool, false)
GODOT_MCP_TOOL(SetWorldEnvironmentTool, false)

// TileMap tools
GODOT_MCP_TOOL(GetTileMapInfoTool, false)
GODOT_MCP_TOOL(SetTileMapCellsTool, false)
GODOT_MCP_TOOL(EraseTileMapCellsTool, false)

// Visualizer tools
GODOT_MCP_TOOL(GetProjectGraphTool, false)

// Workspace / Debugger tools
GODOT_MCP_TOOL(CaptureViewportTool, false)
GODOT_MCP_TOOL(CaptureGameViewportTool, false)
GODOT_MCP_TOOL(ClearConsoleTool, false)
GODOT_MCP_TOOL(GetConsoleOutputTool, false)
GODOT_MCP_TOOL(GetConsoleErrorsTool, false)
GODOT_MCP_TOOL(GetConsoleWarningsTool, false)
GODOT_MCP_TOOL(GetDebuggerStateTool, false)
GODOT_MCP_TOOL(GetDebuggerStatusTool, false)
GODOT_MCP_TOOL(GetDebuggerErrorsTool, false)
GODOT_MCP_TOOL(GetFpsTool, false)
GODOT_MCP_TOOL(GetMemoryUsageTool, false)
GODOT_MCP_TOOL(GetObjectCountTool, false)
GODOT_MCP_TOOL(GetPerformanceMonitorsTool, false)
GODOT_MCP_TOOL(GetPhysicsStatsTool, false)
GODOT_MCP_TOOL(GetRenderStatsTool, false)
GODOT_MCP_TOOL(GetStackTraceTool, false)
GODOT_MCP_TOOL(GetLocalsTool, false)
GODOT_MCP_TOOL(DebuggerBreakTool, false)
GODOT_MCP_TOOL(DebuggerContinueTool, false)
GODOT_MCP_TOOL(DebuggerControlTool, false)
GODOT_MCP_TOOL(DebuggerStepIntoTool, false)
GODOT_MCP_TOOL(DebuggerStepOutTool, false)
GODOT_MCP_TOOL(DebuggerStepOverTool, false)
GODOT_MCP_TOOL(ListBreakpointsTool, false)
GODOT_MCP_TOOL(SetBreakpointTool, false)
GODOT_MCP_TOOL(RemoveBreakpointTool, false)
GODOT_MCP_TOOL(SetWorkspaceTool, false)
GODOT_MCP_TOOL(SetWorkspace2DTool, false)
GODOT_MCP_TOOL(SetWorkspace3DTool, false)
GODOT_MCP_TOOL(SetWorkspaceScriptTool, false)
GODOT_MCP_TOOL(SetWorkspaceAssetLibTool, false)

// Runtime bridge tools
GODOT_MCP_TOOL(GetGameSceneTreeTool, false)
GODOT_MCP_TOOL(GetGameNodePropertyTool, false)
GODOT_MCP_TOOL(SetGameNodePropertyTool, true)
GODOT_MCP_TOOL(CallMethodInGameTool, false)
GODOT_MCP_TOOL(CaptureGameScreenshotTool, false)
GODOT_MCP_TOOL(SimulateGameInputTool, false)

// Runtime lifecycle tools
GODOT_MCP_TOOL(RunProjectTool, false)
GODOT_MCP_TOOL(RunCurrentSceneTool, false)
GODOT_MCP_TOOL(RunSpecificSceneTool, false)
GODOT_MCP_TOOL(StopProjectTool, false)
GODOT_MCP_TOOL(PauseProjectTool, false)
GODOT_MCP_TOOL(SetMovieMakerTool, false)
