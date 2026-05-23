use serde_json::{Value, json};

use godot::prelude::StringName;

use super::{get_root, j2v, pipe, resolve_node, s, v2j};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &["call_method", "get_variable", "set_variable"];

pub struct ScriptHelpersCommands;

impl ScriptHelpersCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for ScriptHelpersCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl super::CommandHandler for ScriptHelpersCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn execute(&self, _args: &Value, _d: &MainThreadDispatcher) -> Result<Value, String> {
        Err("ScriptHelpersCommands::execute should not be called directly".into())
    }
    fn group_name(&self) -> &str {
        "script_helpers"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl ScriptHelpersCommands {
    pub async fn handle_script_helpers_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "call_method" => pipe(d.submit(move || cmd_call_method(&a)).await),
            "get_variable" => pipe(d.submit(move || cmd_get_variable(&a)).await),
            "set_variable" => pipe(d.submit(move || cmd_set_variable(&a)).await),
            _ => Err(format!("Unknown script_helpers tool: {}", tool)),
        }
    }
}

fn cmd_call_method(args: &Value) -> Value {
    let p = s(args, "node_path");
    let method = s(args, "method");
    if method.is_empty() {
        return json!({"error": "missing 'method'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let mut node = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };
    let call_args: Vec<godot::prelude::Variant> = args["args"]
        .as_array()
        .map(|arr| arr.iter().map(j2v).collect())
        .unwrap_or_default();
    let result = node.call(&StringName::from(method.as_str()), &call_args);
    json!({
        "node_path": p,
        "method": method,
        "result": v2j(&result),
        "type": format!("{:?}", result.get_type())
    })
}

fn cmd_get_variable(args: &Value) -> Value {
    let p = s(args, "node_path");
    let var_name = s(args, "variable");
    if var_name.is_empty() {
        return json!({"error": "missing 'variable'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let val = n.get(&StringName::from(var_name.as_str()));
            if val.is_nil() {
                json!({
                    "node_path": p,
                    "variable": var_name,
                    "value": null,
                    "type": "NIL",
                    "hint": format!(
                        "Variable '{}' not found. In editor mode, Object.get() only returns \
                         engine properties and @export script variables. \
                         Use @export var {} in your GDScript to make it accessible.",
                        var_name, var_name
                    )
                })
            } else {
                json!({
                    "node_path": p,
                    "variable": var_name,
                    "value": v2j(&val),
                    "type": format!("{:?}", val.get_type())
                })
            }
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_variable(args: &Value) -> Value {
    let p = s(args, "node_path");
    let var_name = s(args, "variable");
    let val = match args.get("value") {
        Some(v) => v.clone(),
        None => return json!({"error": "missing 'value'"}),
    };
    if var_name.is_empty() {
        return json!({"error": "missing 'variable'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(mut n) => {
            n.set(&StringName::from(var_name.as_str()), &j2v(&val));
            json!({"node_path": p, "variable": var_name, "value": val})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}
