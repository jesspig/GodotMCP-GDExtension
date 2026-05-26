use serde_json::{Value, json};

use godot::builtin::{Color, Vector2};
use godot::classes::Texture2D;
use godot::prelude::{GString, StringName, Variant};
use godot::tools::try_load;

use super::{get_root, get_undo_redo, pipe, resolve_node, s, v2j};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "get_node_position",
    "set_node_position",
    "get_node_rotation",
    "set_node_rotation",
    "get_node_scale",
    "set_node_scale",
    "get_node_visible",
    "set_node_visible",
    "get_node_modulate",
    "set_node_modulate",
    "get_node_z_index",
    "set_node_z_index",
    "get_node_text",
    "set_node_text",
    "get_node_collision_layer",
    "set_node_collision_layer",
    "get_node_collision_mask",
    "set_node_collision_mask",
    "set_node_unique_name",
    "get_node_texture",
    "set_node_texture",
];

pub struct PropertyCommands;

impl PropertyCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for PropertyCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl super::CommandHandler for PropertyCommands {
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
        Box::pin(self.handle_property_tool(tool, args, d))
    }
    fn group_name(&self) -> &str {
        "property"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl PropertyCommands {
    pub async fn handle_property_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "get_node_position" => pipe(d.submit(move || cmd_get_node_position(&a)).await),
            "set_node_position" => pipe(d.submit(move || cmd_set_node_position(&a)).await),
            "get_node_rotation" => pipe(d.submit(move || cmd_get_node_rotation(&a)).await),
            "set_node_rotation" => pipe(d.submit(move || cmd_set_node_rotation(&a)).await),
            "get_node_scale" => pipe(d.submit(move || cmd_get_node_scale(&a)).await),
            "set_node_scale" => pipe(d.submit(move || cmd_set_node_scale(&a)).await),
            "get_node_visible" => pipe(d.submit(move || cmd_get_node_visible(&a)).await),
            "set_node_visible" => pipe(d.submit(move || cmd_set_node_visible(&a)).await),
            "get_node_modulate" => pipe(d.submit(move || cmd_get_node_modulate(&a)).await),
            "set_node_modulate" => pipe(d.submit(move || cmd_set_node_modulate(&a)).await),
            "get_node_z_index" => pipe(d.submit(move || cmd_get_node_z_index(&a)).await),
            "set_node_z_index" => pipe(d.submit(move || cmd_set_node_z_index(&a)).await),
            "get_node_text" => pipe(d.submit(move || cmd_get_node_text(&a)).await),
            "set_node_text" => pipe(d.submit(move || cmd_set_node_text(&a)).await),
            "get_node_collision_layer" => {
                pipe(d.submit(move || cmd_get_node_collision_layer(&a)).await)
            }
            "set_node_collision_layer" => {
                pipe(d.submit(move || cmd_set_node_collision_layer(&a)).await)
            }
            "get_node_collision_mask" => {
                pipe(d.submit(move || cmd_get_node_collision_mask(&a)).await)
            }
            "set_node_collision_mask" => {
                pipe(d.submit(move || cmd_set_node_collision_mask(&a)).await)
            }
            "set_node_unique_name" => pipe(d.submit(move || cmd_set_node_unique_name(&a)).await),
            "get_node_texture" => pipe(d.submit(move || cmd_get_node_texture(&a)).await),
            "set_node_texture" => pipe(d.submit(move || cmd_set_node_texture(&a)).await),
            _ => Err(format!("Unknown property tool: {}", tool)),
        }
    }
}

macro_rules! get_set_prop {
    ($get_fn:ident, $set_fn:ident, $prop:literal, $get_ret:expr, $set_val:expr) => {
        fn $get_fn(args: &Value) -> Value {
            let p = s(args, "node_path");
            let root = match get_root() { Ok(r) => r, Err(e) => return e };
            match resolve_node(&root, p.as_str()) {
                Some(n) => {
                    let val = n.get(&StringName::from($prop));
                    json!({"node_path": p, $prop: $get_ret(&val)})
                }
                None => json!({"error": format!("Node not found: {}", p)}),
            }
        }
        fn $set_fn(args: &Value) -> Value {
            let p = s(args, "node_path");
            let root = match get_root() { Ok(r) => r, Err(e) => return e };
            match resolve_node(&root, p.as_str()) {
                Some(n) => {
                    let val: Variant = $set_val(args);
                    let action = format!("Set {} for {}", $prop, p);
                    super::undoable_set(&n, $prop, &val, &action);
                    let actual = n.get(&StringName::from($prop));
                    json!({"node_path": p, "property": $prop, "value": $get_ret(&actual)})
                }
                None => json!({"error": format!("Node not found: {}", p)}),
            }
        }
    };
}

// ── Position ─────────────────────────────────────────────────────────

fn cmd_get_node_position(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let v: Vector2 = n.get("position").try_to().unwrap_or_default();
            json!({"node_path": p, "x": v.x, "y": v.y})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_node_position(args: &Value) -> Value {
    let p = s(args, "node_path");
    let x = args["x"].as_f64().unwrap_or(0.0) as f32;
    let y = args["y"].as_f64().unwrap_or(0.0) as f32;
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let val = Variant::from(Vector2::new(x, y));
            super::undoable_set(&n, "position", &val, &format!("Set position for {}", p));
            let actual: Vector2 = n.get("position").try_to().unwrap_or_default();
            json!({"node_path": p, "property": "position", "x": actual.x, "y": actual.y})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── Rotation ─────────────────────────────────────────────────────────

fn cmd_get_node_rotation(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let deg: f64 = n.get("rotation_degrees").try_to().unwrap_or(0.0);
            json!({"node_path": p, "degrees": deg})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_node_rotation(args: &Value) -> Value {
    let p = s(args, "node_path");
    let deg = args["degrees"].as_f64().unwrap_or(0.0);
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let val = Variant::from(deg);
            super::undoable_set(
                &n,
                "rotation_degrees",
                &val,
                &format!("Set rotation for {}", p),
            );
            let actual: f64 = n.get("rotation_degrees").try_to().unwrap_or(0.0);
            json!({"node_path": p, "property": "rotation_degrees", "degrees": actual})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── Scale ─────────────────────────────────────────────────────────────

fn cmd_get_node_scale(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let v: Vector2 = n.get("scale").try_to().unwrap_or_default();
            json!({"node_path": p, "x": v.x, "y": v.y})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_node_scale(args: &Value) -> Value {
    let p = s(args, "node_path");
    let x = args["x"].as_f64().unwrap_or(1.0) as f32;
    let y = args["y"].as_f64().unwrap_or(1.0) as f32;
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let val = Variant::from(Vector2::new(x, y));
            super::undoable_set(&n, "scale", &val, &format!("Set scale for {}", p));
            let actual: Vector2 = n.get("scale").try_to().unwrap_or_default();
            json!({"node_path": p, "property": "scale", "x": actual.x, "y": actual.y})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── Visible ───────────────────────────────────────────────────────────

get_set_prop!(
    cmd_get_node_visible,
    cmd_set_node_visible,
    "visible",
    |v: &godot::prelude::Variant| v.try_to::<bool>().unwrap_or(true),
    |args: &Value| godot::prelude::Variant::from(args["visible"].as_bool().unwrap_or(true))
);

// ── Modulate ──────────────────────────────────────────────────────────

fn cmd_get_node_modulate(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let c: Color = n.get("modulate").try_to().unwrap_or_default();
            json!({"node_path": p, "r": c.r, "g": c.g, "b": c.b, "a": c.a})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_node_modulate(args: &Value) -> Value {
    let p = s(args, "node_path");
    let color_str = s(args, "color");
    let (r, g, b, a) = if !color_str.is_empty() {
        match Color::from_string(&color_str) {
            Some(c) => (c.r, c.g, c.b, c.a),
            None => return json!({"error": format!("Invalid color string: '{}'. Use hex like '#ff0000' or a color name.", color_str)}),
        }
    } else {
        (args["r"].as_f64().unwrap_or(1.0) as f32,
         args["g"].as_f64().unwrap_or(1.0) as f32,
         args["b"].as_f64().unwrap_or(1.0) as f32,
         args["a"].as_f64().unwrap_or(1.0) as f32)
    };
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(mut n) => {
            let val = Variant::from(Color::from_rgba(r, g, b, a));
            n.set("modulate", &val);
            super::undoable_set(&n, "modulate", &val, &format!("Set modulate for {}", p));
            let actual: Color = n.get("modulate").try_to().unwrap_or_default();
            json!({"node_path": p, "property": "modulate", "r": actual.r, "g": actual.g, "b": actual.b, "a": actual.a})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── Z-Index ───────────────────────────────────────────────────────────

get_set_prop!(
    cmd_get_node_z_index,
    cmd_set_node_z_index,
    "z_index",
    |v: &godot::prelude::Variant| v.try_to::<i64>().unwrap_or(0),
    |args: &Value| godot::prelude::Variant::from(args["z_index"].as_i64().unwrap_or(0))
);

// ── Text ──────────────────────────────────────────────────────────────

get_set_prop!(
    cmd_get_node_text,
    cmd_set_node_text,
    "text",
    |v: &godot::prelude::Variant| v2j(v),
    |args: &Value| godot::prelude::Variant::from(GString::from(
        args["text"].as_str().unwrap_or("")
    ))
);

// ── Collision layer/mask ──────────────────────────────────────────────

fn check_collision_compatible(n: &godot::obj::Gd<godot::classes::Node>, p: &str) -> Option<Value> {
    if n.is_class("CollisionObject2D") || n.is_class("CollisionObject3D") {
        None
    } else {
        Some(json!({"error": format!(
            "Node '{}' ({}) does not have collision_layer/mask. Requires CollisionObject2D/3D derived node (e.g. CharacterBody2D, Area2D, StaticBody2D, RigidBody2D).",
            p, n.get_class().to_string()
        )}))
    }
}

fn cmd_get_node_collision_layer(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_collision_compatible(&n, &p) {
                return err;
            }
            let val = n.get(&StringName::from("collision_layer"));
            if val.is_nil() {
                return json!({"error": format!("Node '{}' ({}) does not have collision_layer property.", p, n.get_class().to_string())});
            }
            json!({"node_path": p, "collision_layer": val.try_to::<i64>().unwrap_or(1)})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_node_collision_layer(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_collision_compatible(&n, &p) {
                return err;
            }
            let layer = args["layer"].as_i64().unwrap_or(1);
            let val = Variant::from(layer);
            super::undoable_set(
                &n,
                "collision_layer",
                &val,
                &format!("Set collision_layer for {}", p),
            );
            let actual: i64 = n.get("collision_layer").try_to().unwrap_or(1);
            json!({"node_path": p, "property": "collision_layer", "layer": actual})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_get_node_collision_mask(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_collision_compatible(&n, &p) {
                return err;
            }
            let val = n.get(&StringName::from("collision_mask"));
            if val.is_nil() {
                return json!({"error": format!("Node '{}' ({}) does not have collision_mask property.", p, n.get_class().to_string())});
            }
            json!({"node_path": p, "collision_mask": val.try_to::<i64>().unwrap_or(1)})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_node_collision_mask(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if let Some(err) = check_collision_compatible(&n, &p) {
                return err;
            }
            let mask = args["mask"].as_i64().unwrap_or(1);
            let val = Variant::from(mask);
            super::undoable_set(
                &n,
                "collision_mask",
                &val,
                &format!("Set collision_mask for {}", p),
            );
            let actual: i64 = n.get("collision_mask").try_to().unwrap_or(1);
            json!({"node_path": p, "property": "collision_mask", "mask": actual})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── Unique name ───────────────────────────────────────────────────────

fn cmd_set_node_unique_name(args: &Value) -> Value {
    let p = s(args, "node_path");
    let unique = args["unique"].as_bool().unwrap_or(true);
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let mut ur = get_undo_redo();
            let old_unique = n.is_unique_name_in_owner();
            ur.create_action(&format!("Set unique_name for {}", p));
            ur.add_do_method(
                &n.clone(),
                &StringName::from("set_unique_name_in_owner"),
                &[Variant::from(unique)],
            );
            ur.add_undo_method(
                &n.clone(),
                &StringName::from("set_unique_name_in_owner"),
                &[Variant::from(old_unique)],
            );
            ur.commit_action();
            json!({"node_path": p, "unique_name_in_owner": unique})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── Texture ──────────────────────────────────────────────────────────

fn cmd_get_node_texture(args: &Value) -> Value {
    let p = s(args, "node_path");
    let prop = s(args, "property");
    let prop = if prop.is_empty() { "texture" } else { &prop };
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let val = n.get(&StringName::from(prop));
            if val.is_nil() {
                return json!({"error": format!(
                    "Node '{}' ({}) does not have property '{}'.",
                    p, n.get_class().to_string(), prop
                )});
            }
            let tex: Option<godot::obj::Gd<Texture2D>> = val.try_to().ok();
            match tex {
                Some(t) => json!({
                    "node_path": p,
                    "property": prop,
                    "texture_path": t.get_path().to_string()
                }),
                None => json!({
                    "node_path": p,
                    "property": prop,
                    "texture_path": null,
                    "hint": format!("Property '{}' exists but is not a Texture2D (type: {:?})", prop, n.get(&StringName::from(prop)).get_type())
                }),
            }
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_node_texture(args: &Value) -> Value {
    let p = s(args, "node_path");
    let prop = s(args, "property");
    let prop = if prop.is_empty() { "texture" } else { &prop };
    let tex_path = s(args, "texture_path");
    if tex_path.is_empty() {
        return json!({"error": "missing 'texture_path'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let texture = match try_load::<Texture2D>(&tex_path) {
        Ok(t) => t,
        Err(_) => {
            return json!({"error": format!(
                "Failed to load Texture2D from '{}'. Check that the file exists and is a valid image resource.",
                tex_path
            )});
        }
    };
    match resolve_node(&root, p.as_str()) {
        Some(mut n) => {
            let old_val = n.get(&StringName::from(prop));
            n.set(&StringName::from(prop), &Variant::from(texture));
            let check = n.get(&StringName::from(prop));
            if check.is_nil() {
                return json!({"error": format!(
                    "Node '{}' ({}) does not support property '{}'. The value was rejected.",
                    p, n.get_class().to_string(), prop
                )});
            }
            let mut ur = get_undo_redo();
            ur.create_action(&format!("Set {} for {}", prop, p));
            ur.add_do_property(
                &n.clone(),
                &StringName::from(prop),
                &Variant::from(check.clone()),
            );
            ur.add_undo_property(&n.clone(), &StringName::from(prop), &old_val);
            ur.commit_action_ex().execute(false).done();
            json!({
                "node_path": p,
                "property": prop,
                "texture_path": tex_path
            })
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}
