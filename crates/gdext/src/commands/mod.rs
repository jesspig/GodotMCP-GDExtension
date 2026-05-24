pub mod collision;
pub mod find;
pub mod meta;
pub mod node;
pub mod project_settings;
pub mod property;
pub mod property_3d;
pub mod scene;
pub mod script_cs;
pub mod script_gd;
pub mod script_helpers;
pub mod search;
pub mod undo;

use serde_json::{Value, json};

use godot::builtin::{Color, Quaternion, Rect2, Vector2, Vector3, Vector4};
use godot::classes::{DirAccess, EditorInterface, Node, Resource};
use godot::meta::ToGodot;
use godot::obj::Singleton;
use godot::prelude::{GString, NodePath, StringName, Variant};
use godot::tools::try_load;

use crate::dispatcher::MainThreadDispatcher;

pub trait CommandHandler: Send + Sync {
    fn can_handle(&self, tool: &str) -> bool;
    fn execute(&self, args: &Value, dispatcher: &MainThreadDispatcher) -> Result<Value, String>;
    fn group_name(&self) -> &str;
    fn tool_names(&self) -> &[&str];
}

pub fn create_registry() -> Vec<Box<dyn CommandHandler>> {
    vec![
        Box::new(meta::MetaCommands::new()),
        Box::new(node::NodeCommands::new()),
        Box::new(scene::SceneCommands::new()),
        Box::new(script_gd::ScriptGdCommands::new()),
        Box::new(script_cs::ScriptCsCommands::new()),
        Box::new(search::SearchCommands::new()),
        Box::new(undo::UndoCommands::new()),
        Box::new(property_3d::Property3dCommands::new()),
    ]
}

// ── Shared helpers ───────────────────────────────────────────────────

/// Read a string field from JSON args, defaulting to empty string.
pub fn s(args: &Value, key: &str) -> String {
    args[key].as_str().unwrap_or("").to_string()
}

/// Convert an error-shaped JSON Value into Result<Value, String>.
/// Used to pipe `cmd_*` outputs through to the route_tool_call layer.
pub fn pipe(val: Value) -> Result<Value, String> {
    if let Some(e) = val.get("error").and_then(|v| v.as_str()) {
        Err(e.into())
    } else {
        Ok(val)
    }
}

/// Ensure the parent directory of `path` exists.
/// `path` should be a `res://` style resource path (e.g. `res://sub/file.tscn`).
/// Silently succeeds when the parent is `res://` itself (always exists).
/// **Must be called on the main thread.**
pub fn ensure_parent_dir(path: &str) -> bool {
    let Some(slash) = path.rfind('/') else {
        return true;
    };
    let parent = &path[..=slash];
    if parent == "res://" || parent == "res:/" {
        return true;
    }
    if DirAccess::dir_exists_absolute(&GString::from(parent)) {
        return true;
    }
    let Some(mut d) = DirAccess::open(&GString::from("res://")) else {
        return false;
    };
    let sub = parent.strip_prefix("res://").unwrap_or(parent);
    let sub = sub.trim_end_matches('/');
    if sub.is_empty() {
        return true;
    }
    d.make_dir_recursive(&GString::from(sub));
    true
}

/// Resolve a node from a path, accepting root aliases ("", ".", "/", "/root", root name)
/// and "RootName/Child" style paths (which `relative_path()` produces).
pub fn resolve_node(root: &godot::obj::Gd<Node>, path: &str) -> Option<godot::obj::Gd<Node>> {
    let trimmed = path.trim();
    if trimmed.is_empty()
        || trimmed == "."
        || trimmed == "/"
        || trimmed == "/root"
        || root.get_name() == trimmed
        || trimmed == format!("/root/{}", root.get_name())
    {
        return Some(root.clone());
    }
    let effective = {
        let prefix = format!("{}/", root.get_name());
        if trimmed.starts_with(&prefix) {
            trimmed.strip_prefix(&prefix).unwrap()
        } else {
            trimmed
        }
    };
    root.get_node_or_null(&NodePath::from(effective))
}

/// Convert a Godot Variant to serde_json::Value with type recognition for
/// Vector2/3/4, Color, Rect2, Quaternion, and Resource.
pub fn v2j(v: &Variant) -> Value {
    if v.is_nil() {
        return Value::Null;
    }
    if let Ok(b) = v.try_to::<bool>() {
        return json!(b);
    }
    if let Ok(i) = v.try_to::<i64>() {
        return json!(i);
    }
    if let Ok(f) = v.try_to::<f64>() {
        return json!(f);
    }
    if let Ok(gs) = v.try_to::<GString>() {
        return json!(gs.to_string());
    }
    if let Ok(p) = v.try_to::<Vector2>() {
        return json!({"x": p.x, "y": p.y});
    }
    if let Ok(p) = v.try_to::<Vector3>() {
        return json!({"x": p.x, "y": p.y, "z": p.z});
    }
    if let Ok(p) = v.try_to::<Vector4>() {
        return json!({"x": p.x, "y": p.y, "z": p.z, "w": p.w});
    }
    if let Ok(c) = v.try_to::<Color>() {
        return json!({"r": c.r, "g": c.g, "b": c.b, "a": c.a});
    }
    if let Ok(r) = v.try_to::<Rect2>() {
        return json!({
            "position": {"x": r.position.x, "y": r.position.y},
            "size": {"x": r.size.x, "y": r.size.y}
        });
    }
    if let Ok(q) = v.try_to::<Quaternion>() {
        return json!({"x": q.x, "y": q.y, "z": q.z, "w": q.w});
    }
    if let Ok(res) = v.try_to::<godot::obj::Gd<Resource>>() {
        let path = res.get_path().to_string();
        if !path.is_empty() {
            return json!({"resource_path": path});
        }
    }
    json!(v.to_string())
}

fn obj_keys_match(map: &serde_json::Map<String, Value>, expected: &[&str]) -> bool {
    map.len() == expected.len() && expected.iter().all(|k| map.contains_key(*k))
}

fn obj_get_f32(map: &serde_json::Map<String, Value>, key: &str) -> Option<f32> {
    map.get(key).and_then(|v| v.as_f64()).map(|f| f as f32)
}

/// Convert a serde_json::Value to a Godot Variant with type recognition for
/// Vector2/3/4, Color, Rect2, Quaternion, and Resource (via "resource_path" key
/// or "res://"/"user://" string prefix).
pub fn j2v(v: &Value) -> Variant {
    match v {
        Value::Null => Variant::nil(),
        Value::Bool(b) => Variant::from(*b),
        Value::Number(n) => {
            if let Some(i) = n.as_i64() {
                Variant::from(i)
            } else if let Some(f) = n.as_f64() {
                Variant::from(f)
            } else {
                Variant::nil()
            }
        }
        Value::String(str) => {
            if (str.starts_with("res://") || str.starts_with("user://"))
                && let Ok(res) = try_load::<Resource>(str.as_str())
            {
                return Variant::from(res);
            }
            Variant::from(GString::from(str.as_str()))
        }
        Value::Object(map) => {
            if obj_keys_match(map, &["x", "y"])
                && let (Some(x), Some(y)) = (obj_get_f32(map, "x"), obj_get_f32(map, "y"))
            {
                return Variant::from(Vector2::new(x, y));
            }
            if obj_keys_match(map, &["x", "y", "z"])
                && let (Some(x), Some(y), Some(z)) = (
                    obj_get_f32(map, "x"),
                    obj_get_f32(map, "y"),
                    obj_get_f32(map, "z"),
                )
            {
                return Variant::from(Vector3::new(x, y, z));
            }
            if obj_keys_match(map, &["x", "y", "z", "w"])
                && let (Some(x), Some(y), Some(z), Some(w)) = (
                    obj_get_f32(map, "x"),
                    obj_get_f32(map, "y"),
                    obj_get_f32(map, "z"),
                    obj_get_f32(map, "w"),
                )
            {
                return Variant::from(Quaternion::new(x, y, z, w));
            }
            if obj_keys_match(map, &["r", "g", "b", "a"])
                && let (Some(r), Some(g), Some(b), Some(a)) = (
                    obj_get_f32(map, "r"),
                    obj_get_f32(map, "g"),
                    obj_get_f32(map, "b"),
                    obj_get_f32(map, "a"),
                )
            {
                return Variant::from(Color::from_rgba(r, g, b, a));
            }
            if obj_keys_match(map, &["r", "g", "b"])
                && let (Some(r), Some(g), Some(b)) = (
                    obj_get_f32(map, "r"),
                    obj_get_f32(map, "g"),
                    obj_get_f32(map, "b"),
                )
            {
                return Variant::from(Color::from_rgb(r, g, b));
            }
            if obj_keys_match(map, &["position", "size"])
                && let (Some(Value::Object(p)), Some(Value::Object(sz))) =
                    (map.get("position"), map.get("size"))
                && let (Some(px), Some(py), Some(sx), Some(sy)) = (
                    obj_get_f32(p, "x"),
                    obj_get_f32(p, "y"),
                    obj_get_f32(sz, "x"),
                    obj_get_f32(sz, "y"),
                )
            {
                return Variant::from(Rect2::from_components(px, py, sx, sy));
            }
            if let Some(Value::String(path)) = map.get("resource_path")
                && let Ok(res) = try_load::<Resource>(path.as_str())
            {
                return Variant::from(res);
            }
            Variant::from(GString::from(&v.to_string()))
        }
        Value::Array(arr) => {
            let nums: Option<Vec<f32>> = arr.iter().map(|x| x.as_f64().map(|f| f as f32)).collect();
            if let Some(ns) = nums {
                match ns.len() {
                    2 => return Variant::from(Vector2::new(ns[0], ns[1])),
                    3 => return Variant::from(Vector3::new(ns[0], ns[1], ns[2])),
                    4 => return Variant::from(Color::from_rgba(ns[0], ns[1], ns[2], ns[3])),
                    _ => {}
                }
            }
            Variant::from(GString::from(&v.to_string()))
        }
    }
}

/// Globalize a "res://" path to an OS absolute path via ProjectSettings.
/// Returns None if ProjectSettings is unavailable.
/// **Must be called on the main thread.**
pub fn globalize_path(res_path: &str) -> Option<String> {
    use godot::classes::ProjectSettings;
    use godot::obj::Singleton;
    let ps = ProjectSettings::singleton();
    Some(ps.globalize_path(&GString::from(res_path)).to_string())
}

/// StringName from a &str (small convenience).
pub fn sn(s: &str) -> StringName {
    StringName::from(s)
}

/// Compute a scene-relative path like "Pong/Ball" instead of the editor-internal
/// "/root/@EditorNode@.../Root/Pong/Ball".
pub fn relative_path(node: &godot::obj::Gd<Node>, root: &godot::obj::Gd<Node>) -> String {
    let full = node.get_path().to_string();
    let root_full = root.get_path().to_string();
    if full == root_full {
        return root.get_name().to_string();
    }
    if let Some(suffix) = full
        .strip_prefix(&root_full)
        .and_then(|s| s.strip_prefix('/'))
    {
        format!("{}/{}", root.get_name(), suffix)
    } else {
        full
    }
}

/// Boilerplate: get edited scene root or return an error JSON.
pub fn get_root() -> Result<godot::obj::Gd<Node>, Value> {
    EditorInterface::singleton()
        .get_edited_scene_root()
        .ok_or_else(|| json!({"error": "No scene open"}))
}

/// Get the editor's undo/redo manager. Must be called on the main thread.
pub fn get_undo_redo() -> godot::obj::Gd<godot::classes::EditorUndoRedoManager> {
    EditorInterface::singleton()
        .get_editor_undo_redo()
        .expect("EditorUndoRedoManager should be available in editor mode")
}

/// Undoable property change: records old value and sets new value via the editor's undo system.
pub fn undoable_set(
    node: &godot::obj::Gd<Node>,
    property: &str,
    new_value: &Variant,
    action_name: &str,
) {
    let mut ur = get_undo_redo();
    let old_value = node.get(property);
    ur.create_action(action_name);
    ur.add_do_property(node, property, new_value);
    ur.add_undo_property(node, property, &old_value);
    ur.commit_action();
}

/// Mark the current scene as having unsaved changes.
pub fn mark_dirty() {
    EditorInterface::singleton().mark_scene_as_unsaved();
}

/// Recursively fix owners for a subtree (move to shared location).
pub fn fix_owners_recursive(node: &godot::obj::Gd<Node>, owner: &godot::obj::Gd<Node>) {
    let count = node.get_child_count();
    for i in 0..count {
        if let Some(mut child) = node.get_child(i) {
            child.set_owner(owner);
            fix_owners_recursive(&child, owner);
        }
    }
}

/// Controls which UndoRedo actions `node_replace_owner` registers.
#[derive(Clone, Copy)]
pub enum ReplaceOwnerMode {
    Do,
    Undo,
    Both,
}

/// Recursively walk all descendants of `base`. For any node whose `owner == old_owner`
/// (and is not `new_owner` itself), record `set_owner(new_owner)` via UndoRedo.
/// Mirrors Godot editor's `SceneTreeDock::_node_replace_owner`.
pub fn node_replace_owner(
    base: &godot::obj::Gd<Node>,
    old_owner: &godot::obj::Gd<Node>,
    new_owner: &godot::obj::Gd<Node>,
    ur: &mut godot::obj::Gd<godot::classes::EditorUndoRedoManager>,
    mode: ReplaceOwnerMode,
) {
    let count = base.get_child_count();
    for i in 0..count {
        if let Some(child) = base.get_child(i) {
            if child.get_owner() == Some(old_owner.clone()) && child != *new_owner {
                match mode {
                    ReplaceOwnerMode::Do => {
                        ur.add_do_method(
                            &child.clone(),
                            &StringName::from("set_owner"),
                            &[new_owner.to_variant()],
                        );
                    }
                    ReplaceOwnerMode::Undo => {
                        ur.add_undo_method(
                            &child.clone(),
                            &StringName::from("set_owner"),
                            &[old_owner.to_variant()],
                        );
                    }
                    ReplaceOwnerMode::Both => {
                        ur.add_do_method(
                            &child.clone(),
                            &StringName::from("set_owner"),
                            &[new_owner.to_variant()],
                        );
                        ur.add_undo_method(
                            &child.clone(),
                            &StringName::from("set_owner"),
                            &[old_owner.to_variant()],
                        );
                    }
                }
            }
            node_replace_owner(&child, old_owner, new_owner, ur, mode);
        }
    }
}
