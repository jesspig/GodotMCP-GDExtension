use serde_json::{Value, json};

use godot::classes::Node;
use godot::prelude::StringName;

use super::{get_root, pipe, relative_path, s};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "find_nodes_by_name",
    "find_nodes_by_type",
    "find_nodes_by_group",
    "find_nodes_by_script",
];

pub struct FindCommands;

impl FindCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for FindCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl super::CommandHandler for FindCommands {
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
        Box::pin(self.handle_find_tool(tool, args, d))
    }
    fn group_name(&self) -> &str {
        "find"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl FindCommands {
    pub async fn handle_find_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "find_nodes_by_name" => pipe(d.submit(move || cmd_find_by_name(&a)).await),
            "find_nodes_by_type" => pipe(d.submit(move || cmd_find_by_type(&a)).await),
            "find_nodes_by_group" => pipe(d.submit(move || cmd_find_by_group(&a)).await),
            "find_nodes_by_script" => pipe(d.submit(move || cmd_find_by_script(&a)).await),
            _ => Err(format!("Unknown find tool: {}", tool)),
        }
    }
}

fn cmd_find_by_name(args: &Value) -> Value {
    let pattern = s(args, "pattern");
    let max = args["max_results"].as_u64().unwrap_or(100) as usize;
    if pattern.is_empty() {
        return json!({"error": "missing 'pattern'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let mut matches = Vec::new();
    collect_by_name(&root, &pattern, &mut matches, max);
    let truncated = matches.len() >= max;
    json!({"matches": matches, "count": matches.len(), "truncated": truncated})
}

fn cmd_find_by_type(args: &Value) -> Value {
    let node_class = s(args, "node_class");
    let max = args["max_results"].as_u64().unwrap_or(100) as usize;
    if node_class.is_empty() {
        return json!({"error": "missing 'node_class'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let mut matches = Vec::new();
    collect_by_type(&root, &node_class, &mut matches, max);
    let truncated = matches.len() >= max;
    json!({"matches": matches, "count": matches.len(), "truncated": truncated})
}

fn cmd_find_by_group(args: &Value) -> Value {
    let group = s(args, "group");
    let max = args["max_results"].as_u64().unwrap_or(100) as usize;
    if group.is_empty() {
        return json!({"error": "missing 'group'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let mut matches = Vec::new();
    collect_by_group(&root, &group, &mut matches, max);
    let truncated = matches.len() >= max;
    json!({"matches": matches, "count": matches.len(), "truncated": truncated})
}

fn cmd_find_by_script(args: &Value) -> Value {
    let script_path = s(args, "script_path");
    let max = args["max_results"].as_u64().unwrap_or(100) as usize;
    if script_path.is_empty() {
        return json!({"error": "missing 'script_path'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let mut matches = Vec::new();
    collect_by_script(&root, &script_path, &mut matches, max);
    let truncated = matches.len() >= max;
    json!({"matches": matches, "count": matches.len(), "truncated": truncated})
}

fn push_match(n: &godot::obj::Gd<Node>, root: &godot::obj::Gd<Node>) -> Value {
    json!({
        "name": n.get_name().to_string(),
        "type": n.get_class().to_string(),
        "path": relative_path(n, root),
    })
}

fn collect_by_name(n: &godot::obj::Gd<Node>, pattern: &str, out: &mut Vec<Value>, max: usize) {
    if out.len() >= max {
        return;
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(_) => return,
    };
    if n.get_name().to_string().contains(pattern) {
        out.push(push_match(n, &root));
    }
    for i in 0..n.get_child_count() {
        if let Some(c) = n.get_child(i) {
            collect_by_name(&c, pattern, out, max);
        }
    }
}

fn collect_by_type(n: &godot::obj::Gd<Node>, node_class: &str, out: &mut Vec<Value>, max: usize) {
    if out.len() >= max {
        return;
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(_) => return,
    };
    if n.is_class(node_class) {
        out.push(push_match(n, &root));
    }
    for i in 0..n.get_child_count() {
        if let Some(c) = n.get_child(i) {
            collect_by_type(&c, node_class, out, max);
        }
    }
}

fn collect_by_group(n: &godot::obj::Gd<Node>, group: &str, out: &mut Vec<Value>, max: usize) {
    if out.len() >= max {
        return;
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(_) => return,
    };
    if n.is_in_group(&StringName::from(group)) {
        out.push(push_match(n, &root));
    }
    for i in 0..n.get_child_count() {
        if let Some(c) = n.get_child(i) {
            collect_by_group(&c, group, out, max);
        }
    }
}

fn collect_by_script(
    n: &godot::obj::Gd<Node>,
    script_path: &str,
    out: &mut Vec<Value>,
    max: usize,
) {
    if out.len() >= max {
        return;
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(_) => return,
    };
    let script_var = n.get("script");
    if let Ok(res) = script_var.try_to::<godot::obj::Gd<godot::classes::Resource>>() {
        let path = res.get_path().to_string();
        if path.contains(script_path) {
            out.push(push_match(n, &root));
        }
    }
    for i in 0..n.get_child_count() {
        if let Some(c) = n.get_child(i) {
            collect_by_script(&c, script_path, out, max);
        }
    }
}
