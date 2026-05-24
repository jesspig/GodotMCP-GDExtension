use serde_json::{Value, json};

use godot::classes::EditorInterface;
use godot::obj::Singleton;

use super::{get_root, pipe};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &["undo", "redo"];

pub struct UndoCommands;

impl UndoCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for UndoCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl super::CommandHandler for UndoCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn handle<'a>(
        &'a self,
        tool: &'a str,
        args: &'a Value,
        d: &'a MainThreadDispatcher,
    ) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Value, String>> + Send + 'a>>
    {
        Box::pin(self.handle_undo_tool(tool, args, d))
    }
    fn group_name(&self) -> &str {
        "undo"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl UndoCommands {
    pub async fn handle_undo_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "undo" => pipe(d.submit(move || cmd_undo(&a)).await),
            "redo" => pipe(d.submit(move || cmd_redo(&a)).await),
            _ => Err(format!("Unknown undo tool: {}", tool)),
        }
    }
}

fn cmd_undo(_args: &Value) -> Value {
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let ur = EditorInterface::singleton().get_editor_undo_redo();
    let Some(ur_manager) = ur else {
        return json!({"error": "EditorUndoRedoManager not available"});
    };
    let history_id = ur_manager.get_object_history_id(&root);
    let Some(undo_redo) = ur_manager.get_history_undo_redo(history_id) else {
        return json!({"error": "No undo history for current scene"});
    };
    let mut ur_obj = undo_redo;
    if !ur_obj.has_undo() {
        return json!({"success": false, "hint": "Nothing to undo"});
    }
    let name = ur_obj.get_current_action_name().to_string();
    let result = ur_obj.undo();
    json!({"success": result, "action": name})
}

fn cmd_redo(_args: &Value) -> Value {
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let ur = EditorInterface::singleton().get_editor_undo_redo();
    let Some(ur_manager) = ur else {
        return json!({"error": "EditorUndoRedoManager not available"});
    };
    let history_id = ur_manager.get_object_history_id(&root);
    let Some(undo_redo) = ur_manager.get_history_undo_redo(history_id) else {
        return json!({"error": "No redo history for current scene"});
    };
    let mut ur_obj = undo_redo;
    if !ur_obj.has_redo() {
        return json!({"success": false, "hint": "Nothing to redo"});
    }
    // Align with cmd_undo: capture action name before executing redo.
    let name = ur_obj.get_current_action_name().to_string();
    let result = ur_obj.redo();
    json!({"success": result, "action": name})
}
