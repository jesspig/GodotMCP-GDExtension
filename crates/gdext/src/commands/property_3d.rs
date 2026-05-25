use serde_json::{Value, json};

use godot::builtin::Vector3;
use godot::prelude::Variant;

use super::{get_root, pipe, resolve_node, s};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "get_node_position_3d",
    "set_node_position_3d",
    "get_node_rotation_3d",
    "set_node_rotation_3d",
    "get_node_scale_3d",
    "set_node_scale_3d",
];

pub struct Property3dCommands;

impl Property3dCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for Property3dCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl super::CommandHandler for Property3dCommands {
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
        Box::pin(self.handle_property_3d_tool(tool, args, d))
    }
    fn group_name(&self) -> &str {
        "property_3d"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl Property3dCommands {
    pub async fn handle_property_3d_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "get_node_position_3d" => pipe(d.submit(move || cmd_get_position_3d(&a)).await),
            "set_node_position_3d" => pipe(d.submit(move || cmd_set_position_3d(&a)).await),
            "get_node_rotation_3d" => pipe(d.submit(move || cmd_get_rotation_3d(&a)).await),
            "set_node_rotation_3d" => pipe(d.submit(move || cmd_set_rotation_3d(&a)).await),
            "get_node_scale_3d" => pipe(d.submit(move || cmd_get_scale_3d(&a)).await),
            "set_node_scale_3d" => pipe(d.submit(move || cmd_set_scale_3d(&a)).await),
            _ => Err(format!("Unknown property_3d tool: {}", tool)),
        }
    }
}

fn check_node3d(n: &godot::obj::Gd<godot::classes::Node>, p: &str) -> Option<Value> {
    if n.is_class("Node3D") {
        None
    } else {
        Some(json!({"error": format!(
            "Node '{}' ({}) is not a Node3D. 3D position/rotation/scale tools require a Node3D-derived node.",
            p, n.get_class().to_string()
        )}))
    }
}

fn cmd_get_position_3d(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_node3d(&n, &p) {
                return err;
            }
            let v: Vector3 = n.get("position").try_to().unwrap_or_default();
            json!({"node_path": p, "x": v.x, "y": v.y, "z": v.z})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_position_3d(args: &Value) -> Value {
    let p = s(args, "node_path");
    let x = args["x"].as_f64().unwrap_or(0.0) as f32;
    let y = args["y"].as_f64().unwrap_or(0.0) as f32;
    let z = args["z"].as_f64().unwrap_or(0.0) as f32;
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_node3d(&n, &p) {
                return err;
            }
            let val = Variant::from(Vector3::new(x, y, z));
            super::undoable_set(&n, "position", &val, &format!("Set 3D position for {}", p));
            let actual: Vector3 = n.get("position").try_to().unwrap_or_default();
            json!({"node_path": p, "property": "position", "x": actual.x, "y": actual.y, "z": actual.z})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_get_rotation_3d(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_node3d(&n, &p) {
                return err;
            }
            let v: Vector3 = n.get("rotation_degrees").try_to().unwrap_or_default();
            json!({"node_path": p, "x": v.x, "y": v.y, "z": v.z})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_rotation_3d(args: &Value) -> Value {
    let p = s(args, "node_path");
    let x = args["x"].as_f64().unwrap_or(0.0) as f32;
    let y = args["y"].as_f64().unwrap_or(0.0) as f32;
    let z = args["z"].as_f64().unwrap_or(0.0) as f32;
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_node3d(&n, &p) {
                return err;
            }
            let val = Variant::from(Vector3::new(x, y, z));
            super::undoable_set(
                &n,
                "rotation_degrees",
                &val,
                &format!("Set 3D rotation for {}", p),
            );
            let actual: Vector3 = n.get("rotation_degrees").try_to().unwrap_or_default();
            json!({"node_path": p, "property": "rotation_degrees", "x": actual.x, "y": actual.y, "z": actual.z})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_get_scale_3d(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_node3d(&n, &p) {
                return err;
            }
            let v: Vector3 = n.get("scale").try_to().unwrap_or_default();
            json!({"node_path": p, "x": v.x, "y": v.y, "z": v.z})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_scale_3d(args: &Value) -> Value {
    let p = s(args, "node_path");
    let x = args["scale_x"]
        .as_f64()
        .or(args["x"].as_f64())
        .unwrap_or(1.0) as f32;
    let y = args["scale_y"]
        .as_f64()
        .or(args["y"].as_f64())
        .unwrap_or(1.0) as f32;
    let z = args["scale_z"]
        .as_f64()
        .or(args["z"].as_f64())
        .unwrap_or(1.0) as f32;
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_node3d(&n, &p) {
                return err;
            }
            let val = Variant::from(Vector3::new(x, y, z));
            super::undoable_set(&n, "scale", &val, &format!("Set 3D scale for {}", p));
            let actual: Vector3 = n.get("scale").try_to().unwrap_or_default();
            json!({"node_path": p, "property": "scale", "x": actual.x, "y": actual.y, "z": actual.z})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}
