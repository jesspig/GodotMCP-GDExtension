use serde_json::{Value, json};

use godot::classes::{ClassDb, EditorInterface, Node};
use godot::obj::Singleton;
use godot::prelude::{StringName, Variant};
use godot::tools::try_load;

use super::{CommandHandler, j2v, pipe, resolve_node, s, v2j};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "get_scene_tree",
    "get_node_path",
    "get_property_list",
    "get_property",
    "create_node",
    "delete_node",
    "rename_node",
    "set_property",
    "duplicate_node",
    "move_node",
    "attach_script",
    "detach_script",
    "find_nodes",
    "reset_parent",
    "set_as_root",
    "batch_set_property",
];

pub struct NodeCommands;

impl NodeCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for NodeCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for NodeCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn execute(&self, _args: &Value, _d: &MainThreadDispatcher) -> Result<Value, String> {
        Err("NodeCommands::execute should not be called directly".into())
    }
    fn group_name(&self) -> &str {
        "node"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl NodeCommands {
    pub async fn handle_node_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "get_scene_tree" => pipe(d.submit(move || cmd_get_scene_tree(&a)).await),
            "get_node_path" => pipe(d.submit(move || cmd_get_node_path(&a)).await),
            "get_property_list" => pipe(d.submit(move || cmd_get_property_list(&a)).await),
            "get_property" => pipe(d.submit(move || cmd_get_property(&a)).await),
            "create_node" => pipe(d.submit(move || cmd_create_node(&a)).await),
            "delete_node" => pipe(d.submit(move || cmd_delete_node(&a)).await),
            "rename_node" => pipe(d.submit(move || cmd_rename_node(&a)).await),
            "set_property" => pipe(d.submit(move || cmd_set_property(&a)).await),
            "duplicate_node" => pipe(d.submit(move || cmd_duplicate_node(&a)).await),
            "move_node" => pipe(d.submit(move || cmd_move_node(&a)).await),
            "attach_script" => pipe(d.submit(move || cmd_attach_script(&a)).await),
            "detach_script" => pipe(d.submit(move || cmd_detach_script(&a)).await),
            "find_nodes" => pipe(d.submit(move || cmd_find_nodes(&a)).await),
            "reset_parent" => pipe(d.submit(move || cmd_reset_parent(&a)).await),
            "set_as_root" => pipe(d.submit(move || cmd_set_as_root(&a)).await),
            "batch_set_property" => pipe(d.submit(move || cmd_batch_set_property(&a)).await),
            _ => Err(format!("Unknown node tool: {}", tool)),
        }
    }
}

// ── Read commands ────────────────────────────────────────────────────

fn cmd_get_scene_tree(args: &Value) -> Value {
    let max = args["max_depth"].as_u64().unwrap_or(10) as i32;
    let ei = EditorInterface::singleton();
    match ei.get_edited_scene_root() {
        Some(root) => serialize_tree(&root, 0, max),
        None => json!({"error": "No scene open"}),
    }
}

fn cmd_get_node_path(args: &Value) -> Value {
    let p = s(args, "node_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => json!({"path": n.get_path().to_string()}),
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_get_property_list(args: &Value) -> Value {
    let p = s(args, "node_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let list: Vec<Value> = n
                .get_property_list()
                .iter_shared()
                .map(|d| {
                    let name = d.get(&Variant::from("name")).unwrap_or(Variant::nil());
                    let tid = d.get(&Variant::from("type")).unwrap_or(Variant::nil());
                    json!({"name": name.to_string(), "type": tid.to::<i64>()})
                })
                .collect();
            json!({"properties": list})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_get_property(args: &Value) -> Value {
    let p = s(args, "node_path");
    let prop = s(args, "property");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => json!({"value": v2j(&n.get(&StringName::from(prop.as_str())))}),
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── Node write commands ──────────────────────────────────────────────

fn cmd_create_node(args: &Value) -> Value {
    let parent_p = s(args, "parent_path");
    let node_type = s(args, "node_type");
    let name = s(args, "name");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    let mut parent = match resolve_node(&root, parent_p.as_str()) {
        Some(p) => p,
        None => return json!({"error": format!("Parent not found: {}", parent_p)}),
    };
    let variant = ClassDb::singleton().instantiate(&StringName::from(node_type.as_str()));
    match variant.try_to::<godot::prelude::Gd<Node>>() {
        Ok(mut node) => {
            node.set_name(&StringName::from(name.as_str()));
            parent.add_child(&node);
            node.set_owner(&root);
            json!({"path": format!("{}/{}", parent_p, name)})
        }
        Err(_) => json!({"error": format!("Cannot instantiate type: {}", node_type)}),
    }
}

fn cmd_delete_node(args: &Value) -> Value {
    let p = s(args, "node_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(mut n) => {
            n.queue_free();
            json!({"deleted": p})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_rename_node(args: &Value) -> Value {
    let p = s(args, "node_path");
    let new_name = s(args, "new_name");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(mut n) => {
            n.set_name(&StringName::from(new_name.as_str()));
            json!({"new_path": n.get_path().to_string()})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_property(args: &Value) -> Value {
    let p = s(args, "node_path");
    let prop = s(args, "property");
    let val = match args.get("value") {
        Some(v) => v.clone(),
        None => return json!({"error": "missing 'value'"}),
    };
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(mut n) => {
            n.set(&StringName::from(prop.as_str()), &j2v(&val));
            json!({"node_path": p, "property": prop, "value": val})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_duplicate_node(args: &Value) -> Value {
    let p = s(args, "node_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let mut parent = n.get_parent().unwrap();
            match n.duplicate_node_ex().done_or_null() {
                Some(mut dup) => {
                    let nm = format!("{}_copy", n.get_name());
                    dup.set_name(&StringName::from(nm.as_str()));
                    parent.add_child(&dup);
                    dup.set_owner(&root);
                    json!({"new_path": dup.get_path().to_string()})
                }
                None => json!({"error": "Failed to duplicate node"}),
            }
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_move_node(args: &Value) -> Value {
    let p = s(args, "node_path");
    let new_p = s(args, "new_parent_path");
    let idx = args["position_index"].as_i64();
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    let mut node = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };
    let mut new_parent = match resolve_node(&root, new_p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("New parent not found: {}", new_p)}),
    };
    let mut old_parent = node.get_parent().unwrap();
    old_parent.remove_child(&node);
    new_parent.add_child(&node);
    node.set_owner(&root);
    if let Some(i) = idx {
        new_parent.move_child(&node, i as i32);
    }
    json!({"new_path": node.get_path().to_string()})
}

// ── Script tools ─────────────────────────────────────────────────────

fn cmd_attach_script(args: &Value) -> Value {
    let p = s(args, "node_path");
    let sp = s(args, "script_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(mut n) => match try_load::<godot::classes::Script>(sp.as_str()) {
            Ok(script) => {
                n.set("script", &Variant::from(script));
                json!({"node_path": p, "script_path": sp})
            }
            Err(e) => json!({"error": format!("Script '{}' could not be loaded: {}", sp, e)}),
        },
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_detach_script(args: &Value) -> Value {
    let p = s(args, "node_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(mut n) => {
            n.set("script", &Variant::nil());
            json!({"node_path": p, "script": null})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_find_nodes(args: &Value) -> Value {
    let q = s(args, "query");
    let method = args["search_method"].as_str().unwrap_or("name").to_string();
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    let mut matches = Vec::new();
    collect_nodes(&root, &q, &method, &mut matches, 0, 50);
    json!({"matches": matches, "count": matches.len()})
}

// ── Advanced ─────────────────────────────────────────────────────────

fn cmd_reset_parent(args: &Value) -> Value {
    let p = s(args, "node_path");
    let np = s(args, "new_parent_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    let mut node = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };
    let new_parent = match resolve_node(&root, np.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("New parent not found: {}", np)}),
    };
    node.reparent(&new_parent);
    json!({"node_path": p, "new_parent": np})
}

fn cmd_set_as_root(args: &Value) -> Value {
    let p = s(args, "node_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(node) => {
            let mut tree = root.get_tree();
            tree.set_edited_scene_root(&node);
            json!({"root": p})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_batch_set_property(args: &Value) -> Value {
    let paths: Vec<String> = match args["node_paths"].as_array() {
        Some(a) => a
            .iter()
            .map(|v| v.as_str().unwrap_or("").to_string())
            .collect(),
        None => return json!({"error": "missing 'node_paths' array"}),
    };
    let prop = s(args, "property");
    let val = match args.get("value") {
        Some(v) => v.clone(),
        None => return json!({"error": "missing 'value'"}),
    };
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    let results: Vec<Value> = paths
        .iter()
        .map(|p| match resolve_node(&root, p.as_str()) {
            Some(mut n) => {
                n.set(&StringName::from(prop.as_str()), &j2v(&val));
                json!({"node_path": p, "status": "ok"})
            }
            None => json!({"node_path": p, "status": "error", "message": "not found"}),
        })
        .collect();
    json!({"results": results})
}

// ── Tree serialization ───────────────────────────────────────────────

fn serialize_tree(n: &godot::prelude::Gd<Node>, depth: i32, max: i32) -> Value {
    if depth > max {
        return json!({"name": "...", "type": "truncated"});
    }
    let children: Vec<Value> = (0..n.get_child_count())
        .filter_map(|i| n.get_child(i).map(|c| serialize_tree(&c, depth + 1, max)))
        .collect();
    json!({
        "name": n.get_name().to_string(),
        "type": n.get_class().to_string(),
        "path": n.get_path().to_string(),
        "children": children,
    })
}

fn collect_nodes(
    n: &godot::prelude::Gd<Node>,
    q: &str,
    method: &str,
    out: &mut Vec<Value>,
    depth: i32,
    max: i32,
) {
    if depth > max || out.len() >= 100 {
        return;
    }
    let name = n.get_name().to_string();
    let class = n.get_class().to_string();
    let path = n.get_path().to_string();
    let hit = match method {
        "type" => class.contains(q),
        "path" => path.contains(q),
        _ => name.contains(q),
    };
    if hit {
        out.push(json!({"name": name, "type": class, "path": path}));
    }
    for i in 0..n.get_child_count() {
        if let Some(c) = n.get_child(i) {
            collect_nodes(&c, q, method, out, depth + 1, max);
        }
    }
}
