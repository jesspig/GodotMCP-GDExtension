use serde_json::{Value, json};

use godot::classes::{ClassDb, Engine, Node};
use godot::meta::ToGodot;
use godot::obj::Singleton;
use godot::prelude::{GString, StringName, Variant};
use godot::tools::try_load;

use super::{
    ReplaceOwnerMode, get_root, get_undo_redo, j2v, mark_dirty, node_replace_owner, pipe,
    relative_path, resolve_node, s, v2j,
};
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
    "reset_parent",
    "set_as_root",
    "batch_set_property",
    "add_node_to_group",
    "remove_node_from_group",
    "set_node_transform_2d",
    "set_node_transform_3d",
    "get_node_info",
    "get_script_variables",
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

impl super::CommandHandler for NodeCommands {
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
        Box::pin(self.handle_node_tool(tool, args, d))
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
            "reset_parent" => pipe(d.submit(move || cmd_reset_parent(&a)).await),
            "set_as_root" => pipe(d.submit(move || cmd_set_as_root(&a)).await),
            "batch_set_property" => pipe(d.submit(move || cmd_batch_set_property(&a)).await),
            "add_node_to_group" => pipe(d.submit(move || cmd_add_node_to_group(&a)).await),
            "remove_node_from_group" => {
                pipe(d.submit(move || cmd_remove_node_from_group(&a)).await)
            }
            "set_node_transform_2d" => pipe(d.submit(move || cmd_set_node_transform_2d(&a)).await),
            "set_node_transform_3d" => pipe(d.submit(move || cmd_set_node_transform_3d(&a)).await),
            "get_node_info" => pipe(d.submit(move || cmd_get_node_info(&a)).await),
            "get_script_variables" => pipe(d.submit(move || cmd_get_script_variables(&a)).await),
            _ => Err(format!("Unknown node tool: {}", tool)),
        }
    }
}

// ── Read commands ────────────────────────────────────────────────────

fn cmd_get_scene_tree(args: &Value) -> Value {
    let max = args["max_depth"].as_u64().unwrap_or(10) as i32;
    match get_root() {
        Ok(root) => serialize_tree(&root, &root, 0, max),
        Err(e) => e,
    }
}

fn cmd_get_node_path(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => json!({"path": relative_path(&n, &root)}),
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_get_property_list(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let list: Vec<Value> = n
                .get_property_list()
                .iter_shared()
                .filter_map(|d| {
                    let name_v = d.get(&Variant::from("name"))?;
                    let name = name_v.to_string();
                    let usage: i64 = d.get(&Variant::from("usage"))?.try_to().unwrap_or(0);
                    if usage & 0x00000040 != 0 {
                        return None;
                    }
                    let type_id: i64 = d.get(&Variant::from("type"))?.try_to().unwrap_or(0);
                    let type_name = variant_type_name(type_id);
                    let hint: i64 = d.get(&Variant::from("hint"))?.try_to().unwrap_or(0);
                    let hint_string = d
                        .get(&Variant::from("hint_string"))
                        .map(|v| v.to_string())
                        .unwrap_or_default();
                    Some(json!({
                        "name": name,
                        "type": type_name,
                        "hint": hint,
                        "hint_string": hint_string,
                    }))
                })
                .collect();
            json!({"properties": list})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn variant_type_name(type_id: i64) -> &'static str {
    match type_id {
        0 => "NIL",
        1 => "bool",
        2 => "int",
        3 => "float",
        4 => "String",
        5 => "Vector2",
        6 => "Vector2i",
        7 => "Rect2",
        8 => "Rect2i",
        9 => "Vector3",
        10 => "Vector3i",
        11 => "Transform2D",
        12 => "Vector4",
        13 => "Vector4i",
        14 => "Plane",
        15 => "Quaternion",
        16 => "AABB",
        17 => "Basis",
        18 => "Transform3D",
        19 => "Projection",
        20 => "Color",
        21 => "StringName",
        22 => "NodePath",
        23 => "RID",
        24 => "Object",
        25 => "Dictionary",
        26 => "Array",
        27 => "PackedByteArray",
        28 => "PackedInt32Array",
        29 => "PackedInt64Array",
        30 => "PackedFloat32Array",
        31 => "PackedFloat64Array",
        32 => "PackedStringArray",
        33 => "PackedVector2Array",
        34 => "PackedVector3Array",
        35 => "PackedColorArray",
        _ => "Unknown",
    }
}

fn cmd_get_property(args: &Value) -> Value {
    let p = s(args, "node_path");
    let prop = s(args, "property");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
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
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let parent = match resolve_node(&root, parent_p.as_str()) {
        Some(p) => p,
        None => return json!({"error": format!("Parent not found: {}", parent_p)}),
    };
    let variant = ClassDb::singleton().instantiate(&StringName::from(node_type.as_str()));
    match variant.try_to::<godot::prelude::Gd<Node>>() {
        Ok(mut node) => {
            node.set_name(&StringName::from(name.as_str()));
            let mut ur = get_undo_redo();
            ur.create_action(&format!("Create Node: {}", name));
            ur.add_do_method(
                &parent.clone(),
                &StringName::from("add_child"),
                &[node.to_variant()],
            );
            ur.add_do_method(
                &node.clone(),
                &StringName::from("set_owner"),
                &[root.to_variant()],
            );
            ur.add_do_reference(&node.clone());
            ur.add_undo_method(
                &parent.clone(),
                &StringName::from("remove_child"),
                &[node.to_variant()],
            );
            ur.commit_action();
            json!({"path": relative_path(&node, &root)})
        }
        Err(_) => json!({"error": format!("Cannot instantiate type: {}", node_type)}),
    }
}

fn cmd_delete_node(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let mut n = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };
    if n == root {
        return json!({"error": "Cannot delete the scene root node. Use open_scene to switch to a different scene."});
    }
    let parent = match n.get_parent() {
        Some(p) => p,
        None => return json!({"error": "Node has no parent"}),
    };
    let idx = n.get_index();
    let saved = match n.duplicate_node_ex().done_or_null() {
        Some(s) => s,
        None => return json!({"error": "Failed to duplicate node for undo backup"}),
    };

    let mut ur = get_undo_redo();
    ur.create_action(&format!("Delete Node: {}", p));
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("add_child"),
        &[saved.to_variant()],
    );
    ur.add_undo_method(
        &saved.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("move_child"),
        &[saved.to_variant(), Variant::from(idx as i64)],
    );
    ur.add_undo_reference(&saved.clone());
    ur.commit_action_ex().execute(false).done();

    let mut parent = parent;
    parent.remove_child(&n);
    n.queue_free();
    json!({"deleted": p})
}

fn cmd_rename_node(args: &Value) -> Value {
    let p = s(args, "node_path");
    let new_name = s(args, "new_name");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let old_name = n.get_name().to_string();
            let mut ur = get_undo_redo();
            ur.create_action(&format!("Rename {} to {}", old_name, new_name));
            ur.add_do_property(
                &n.clone(),
                &StringName::from("name"),
                &Variant::from(GString::from(new_name.as_str())),
            );
            ur.add_undo_property(
                &n.clone(),
                &StringName::from("name"),
                &Variant::from(GString::from(old_name.as_str())),
            );
            ur.commit_action();
            json!({"new_path": relative_path(&n, &root)})
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
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let gv = j2v(&val);
            super::undoable_set(&n, &prop, &gv, &format!("Set {} for {}", prop, p));
            let actual = n.get(&StringName::from(prop.as_str()));
            json!({"node_path": p, "property": prop, "value": v2j(&actual)})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_duplicate_node(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let parent = n.get_parent().unwrap();
            match n.duplicate_node_ex().done_or_null() {
                Some(mut dup) => {
                    let nm = format!("{}_copy", n.get_name());
                    dup.set_name(&StringName::from(nm.as_str()));
                    let mut ur = get_undo_redo();
                    ur.create_action(&format!("Duplicate Node: {}", p));
                    ur.add_do_method(
                        &parent.clone(),
                        &StringName::from("add_child"),
                        &[dup.to_variant()],
                    );
                    ur.add_do_method(
                        &dup.clone(),
                        &StringName::from("set_owner"),
                        &[root.to_variant()],
                    );
                    ur.add_do_reference(&dup.clone());
                    ur.add_undo_method(
                        &parent.clone(),
                        &StringName::from("remove_child"),
                        &[dup.to_variant()],
                    );
                    ur.commit_action();
                    json!({"new_path": relative_path(&dup, &root)})
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
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let node = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };
    let new_parent = match resolve_node(&root, new_p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("New parent not found: {}", new_p)}),
    };
    let old_parent = node.get_parent().unwrap();
    let old_idx = node.get_index();

    let mut ur = get_undo_redo();
    ur.create_action(&format!("Move Node: {}", p));
    ur.add_do_method(
        &old_parent.clone(),
        &StringName::from("remove_child"),
        &[node.to_variant()],
    );
    ur.add_do_method(
        &new_parent.clone(),
        &StringName::from("add_child"),
        &[node.to_variant()],
    );
    ur.add_do_method(
        &node.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    if let Some(i) = idx {
        ur.add_do_method(
            &new_parent.clone(),
            &StringName::from("move_child"),
            &[node.to_variant(), Variant::from(i)],
        );
    }
    ur.add_undo_method(
        &new_parent.clone(),
        &StringName::from("remove_child"),
        &[node.to_variant()],
    );
    ur.add_undo_method(
        &old_parent.clone(),
        &StringName::from("add_child"),
        &[node.to_variant()],
    );
    ur.add_undo_method(
        &node.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    ur.add_undo_method(
        &old_parent.clone(),
        &StringName::from("move_child"),
        &[node.to_variant(), Variant::from(old_idx as i64)],
    );
    ur.commit_action();

    json!({"new_path": relative_path(&node, &root)})
}

// ── Script tools ─────────────────────────────────────────────────────

fn cmd_attach_script(args: &Value) -> Value {
    let p = s(args, "node_path");
    let sp = s(args, "script_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => match try_load::<godot::classes::Script>(sp.as_str()) {
            Ok(script) => {
                let old_script = n.get("script");
                let mut ur = get_undo_redo();
                ur.create_action(&format!("Attach Script: {}", sp));
                ur.add_do_property(
                    &n.clone(),
                    &StringName::from("script"),
                    &Variant::from(script),
                );
                ur.add_undo_property(&n.clone(), &StringName::from("script"), &old_script);
                ur.commit_action();
                json!({"node_path": p, "script_path": sp})
            }
            Err(e) => json!({"error": format!("Script '{}' could not be loaded: {}", sp, e)}),
        },
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_detach_script(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let old_script = n.get("script");
            let mut ur = get_undo_redo();
            ur.create_action(&format!("Detach Script from {}", p));
            ur.add_do_property(&n.clone(), &StringName::from("script"), &Variant::nil());
            ur.add_undo_property(&n.clone(), &StringName::from("script"), &old_script);
            ur.commit_action();
            json!({"node_path": p, "script": null})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── Advanced ─────────────────────────────────────────────────────────

fn cmd_reset_parent(args: &Value) -> Value {
    let p = s(args, "node_path");
    let np = s(args, "new_parent_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let node = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };
    let new_parent = match resolve_node(&root, np.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("New parent not found: {}", np)}),
    };
    let old_parent = node.get_parent().unwrap();
    let old_idx = node.get_index();

    let mut ur = get_undo_redo();
    ur.create_action(&format!("Reparent {} to {}", p, np));
    ur.add_do_method(
        &old_parent.clone(),
        &StringName::from("remove_child"),
        &[node.to_variant()],
    );
    ur.add_do_method(
        &new_parent.clone(),
        &StringName::from("add_child"),
        &[node.to_variant()],
    );
    ur.add_do_method(
        &node.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    ur.add_undo_method(
        &new_parent.clone(),
        &StringName::from("remove_child"),
        &[node.to_variant()],
    );
    ur.add_undo_method(
        &old_parent.clone(),
        &StringName::from("add_child"),
        &[node.to_variant()],
    );
    ur.add_undo_method(
        &old_parent.clone(),
        &StringName::from("move_child"),
        &[node.to_variant(), Variant::from(old_idx as i64)],
    );
    ur.add_undo_method(
        &node.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    ur.commit_action();

    json!({"node_path": relative_path(&node, &root), "new_parent": np})
}

fn find_editor_node(
    tree: &godot::obj::Gd<godot::classes::SceneTree>,
) -> Option<godot::obj::Gd<godot::classes::Object>> {
    if let Some(engine_singleton) =
        Engine::singleton().get_singleton(&StringName::from("EditorNode"))
    {
        return Some(engine_singleton);
    }
    let root_node = tree.get_root()?;
    let count = root_node.get_child_count();
    for i in 0..count {
        if let Some(child) = root_node.get_child(i) {
            let class_name = child.get_class().to_string();
            if class_name.contains("EditorNode") {
                return Some(child.upcast());
            }
        }
    }
    None
}

fn cmd_set_as_root(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let node = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };
    if node == root {
        return json!({"root": p, "hint": "Node is already the root"});
    }

    let node_owner = node.get_owner();
    if node_owner.as_ref() != Some(&root) {
        return json!({"error": format!(
            "Node '{}' does not belong to the current scene root (owner: {}). Only direct children of the scene can be set as root.",
            p, node_owner.map(|o| o.get_name().to_string()).unwrap_or_else(|| "null".into())
        )});
    }

    let sf = node.get_scene_file_path();
    if !sf.is_empty() {
        return json!({"error": format!(
            "Node '{}' is an instanced scene (source: {}). Cannot set an instanced node as root. Use scene_to_branch first to make it local.",
            p, sf.to_string()
        )});
    }

    let old_root_name = root.get_name().to_string();
    let old_scene_path = root.get_scene_file_path().to_string();

    let parent = match node.get_parent() {
        Some(p) => p,
        None => return json!({"error": "Node has no parent"}),
    };
    let old_idx = node.get_index();

    let tree = root.get_tree();
    let editor_node = find_editor_node(&tree);

    let mut ur = get_undo_redo();
    ur.create_action(&format!("Set '{}' as Scene Root", node.get_name()));

    // ── Do methods (order matches Godot scene_tree_dock.cpp TOOL_MAKE_ROOT) ──

    ur.add_do_method(
        &parent.clone(),
        &StringName::from("remove_child"),
        &[node.to_variant()],
    );
    match &editor_node {
        Some(en) => {
            ur.add_do_method(
                en,
                &StringName::from("set_edited_scene"),
                &[node.to_variant()],
            );
        }
        None => {
            ur.add_do_method(
                &tree,
                &StringName::from("set_edited_scene_root"),
                &[node.to_variant()],
            );
        }
    }
    ur.add_do_method(
        &node.clone(),
        &StringName::from("add_child"),
        &[root.to_variant()],
    );
    if !old_scene_path.is_empty() {
        ur.add_do_method(
            &node.clone(),
            &StringName::from("set_scene_file_path"),
            &[Variant::from(GString::from(old_scene_path.as_str()))],
        );
        ur.add_do_method(
            &root.clone(),
            &StringName::from("set_scene_file_path"),
            &[Variant::from(GString::from(""))],
        );
    }
    ur.add_do_method(
        &node.clone(),
        &StringName::from("set_owner"),
        &[Variant::nil()],
    );
    ur.add_do_method(
        &root.clone(),
        &StringName::from("set_owner"),
        &[node.to_variant()],
    );
    ur.add_do_method(
        &node.clone(),
        &StringName::from("set_unique_name_in_owner"),
        &[Variant::from(false)],
    );
    node_replace_owner(&root, &root, &node, &mut ur, ReplaceOwnerMode::Do);

    // ── Undo methods (order matches Godot scene_tree_dock.cpp TOOL_MAKE_ROOT) ──

    if !old_scene_path.is_empty() {
        ur.add_undo_method(
            &root.clone(),
            &StringName::from("set_scene_file_path"),
            &[Variant::from(GString::from(old_scene_path.as_str()))],
        );
        ur.add_undo_method(
            &node.clone(),
            &StringName::from("set_scene_file_path"),
            &[Variant::from(GString::from(""))],
        );
    }
    ur.add_undo_method(
        &node.clone(),
        &StringName::from("remove_child"),
        &[root.to_variant()],
    );
    match &editor_node {
        Some(en) => {
            ur.add_undo_method(
                en,
                &StringName::from("set_edited_scene"),
                &[root.to_variant()],
            );
        }
        None => {
            ur.add_undo_method(
                &tree,
                &StringName::from("set_edited_scene_root"),
                &[root.to_variant()],
            );
        }
    }
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("add_child"),
        &[node.to_variant()],
    );
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("move_child"),
        &[node.to_variant(), Variant::from(old_idx as i64)],
    );
    ur.add_undo_method(
        &root.clone(),
        &StringName::from("set_owner"),
        &[Variant::nil()],
    );
    ur.add_undo_method(
        &node.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    ur.add_undo_method(
        &node.clone(),
        &StringName::from("set_unique_name_in_owner"),
        &[Variant::from(node.is_unique_name_in_owner())],
    );
    node_replace_owner(&root, &root, &root, &mut ur, ReplaceOwnerMode::Undo);

    ur.commit_action();

    let ei = godot::classes::EditorInterface::singleton();
    let actual = ei.get_edited_scene_root();
    match actual {
        Some(new_root) if new_root == node => {
            let mut old_root = root;
            old_root.set_name(&StringName::from(old_root_name.as_str()));
            mark_dirty();
            json!({"root": relative_path(&new_root, &new_root)})
        }
        Some(new_root) => json!({"error": format!(
            "Failed to set '{}' as scene root. Current root is '{}' ({}).",
            p, new_root.get_name().to_string(), new_root.get_class().to_string()
        )}),
        None => {
            json!({"error": format!("Failed to set '{}' as scene root. Scene root is now empty.", p)})
        }
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
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let gv = j2v(&val);
    let results: Vec<Value> = paths
        .iter()
        .map(|p| match resolve_node(&root, p.as_str()) {
            Some(n) => {
                super::undoable_set(&n, &prop, &gv, &format!("Batch set {} for {}", prop, p));
                let actual = n.get(&StringName::from(prop.as_str()));
                json!({"node_path": p, "status": "ok", "value": v2j(&actual)})
            }
            None => json!({"node_path": p, "status": "error", "message": "not found"}),
        })
        .collect();
    json!({"results": results})
}

// ── Group management ─────────────────────────────────────────────────

fn cmd_add_node_to_group(args: &Value) -> Value {
    let p = s(args, "node_path");
    let group = s(args, "group");
    if group.is_empty() {
        return json!({"error": "missing 'group'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let in_group = n.is_in_group(&StringName::from(group.as_str()));
            if in_group {
                return json!({"node_path": p, "group": group, "hint": "Node is already in this group"});
            }
            let mut ur = get_undo_redo();
            ur.create_action(&format!("Add {} to group {}", p, group));
            ur.add_do_method(
                &n.clone(),
                &StringName::from("add_to_group"),
                &[Variant::from(GString::from(group.as_str()))],
            );
            ur.add_undo_method(
                &n.clone(),
                &StringName::from("remove_from_group"),
                &[Variant::from(GString::from(group.as_str()))],
            );
            ur.commit_action();
            json!({"node_path": p, "group": group})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_remove_node_from_group(args: &Value) -> Value {
    let p = s(args, "node_path");
    let group = s(args, "group");
    if group.is_empty() {
        return json!({"error": "missing 'group'"});
    }
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let in_group = n.is_in_group(&StringName::from(group.as_str()));
            if !in_group {
                return json!({"node_path": p, "group": group, "hint": "Node is not in this group"});
            }
            let mut ur = get_undo_redo();
            ur.create_action(&format!("Remove {} from group {}", p, group));
            ur.add_do_method(
                &n.clone(),
                &StringName::from("remove_from_group"),
                &[Variant::from(GString::from(group.as_str()))],
            );
            ur.add_undo_method(
                &n.clone(),
                &StringName::from("add_to_group"),
                &[Variant::from(GString::from(group.as_str()))],
            );
            ur.commit_action();
            json!({"node_path": p, "group": group})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── New convenience tools ────────────────────────────────────────────

fn cmd_set_node_transform_2d(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let old_pos: godot::builtin::Vector2 = n.get("position").try_to().unwrap_or_default();
            let old_rot: f64 = n.get("rotation_degrees").try_to().unwrap_or(0.0);
            let old_scale: godot::builtin::Vector2 = n.get("scale").try_to().unwrap_or_default();

            let x = args["x"].as_f64().unwrap_or(old_pos.x as f64) as f32;
            let y = args["y"].as_f64().unwrap_or(old_pos.y as f64) as f32;
            let rot = args["degrees"].as_f64().unwrap_or(old_rot);
            let sx = args["scale_x"].as_f64().unwrap_or(old_scale.x as f64) as f32;
            let sy = args["scale_y"].as_f64().unwrap_or(old_scale.y as f64) as f32;

            let mut ur = get_undo_redo();
            ur.create_action(&format!("Set 2D Transform for {}", p));
            ur.add_do_property(
                &n.clone(),
                &StringName::from("position"),
                &Variant::from(godot::builtin::Vector2::new(x, y)),
            );
            ur.add_do_property(
                &n.clone(),
                &StringName::from("rotation_degrees"),
                &Variant::from(rot),
            );
            ur.add_do_property(
                &n.clone(),
                &StringName::from("scale"),
                &Variant::from(godot::builtin::Vector2::new(sx, sy)),
            );
            ur.add_undo_property(
                &n.clone(),
                &StringName::from("position"),
                &Variant::from(old_pos),
            );
            ur.add_undo_property(
                &n.clone(),
                &StringName::from("rotation_degrees"),
                &Variant::from(old_rot),
            );
            ur.add_undo_property(
                &n.clone(),
                &StringName::from("scale"),
                &Variant::from(old_scale),
            );
            ur.commit_action();

            json!({"node_path": p, "x": x, "y": y, "degrees": rot, "scale_x": sx, "scale_y": sy})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_set_node_transform_3d(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            if !n.is_class("Node3D") {
                return json!({"error": format!("Node '{}' ({}) is not a Node3D.", p, n.get_class().to_string())});
            }
            let old_pos: godot::builtin::Vector3 = n.get("position").try_to().unwrap_or_default();
            let old_rot: godot::builtin::Vector3 =
                n.get("rotation_degrees").try_to().unwrap_or_default();
            let old_scale: godot::builtin::Vector3 = n.get("scale").try_to().unwrap_or_default();

            let px = args["x"].as_f64().unwrap_or(old_pos.x as f64) as f32;
            let py = args["y"].as_f64().unwrap_or(old_pos.y as f64) as f32;
            let pz = args["z"].as_f64().unwrap_or(old_pos.z as f64) as f32;
            let rx = args["rot_x"].as_f64().unwrap_or(old_rot.x as f64) as f32;
            let ry = args["rot_y"].as_f64().unwrap_or(old_rot.y as f64) as f32;
            let rz = args["rot_z"].as_f64().unwrap_or(old_rot.z as f64) as f32;
            let sx = args["scale_x"].as_f64().unwrap_or(old_scale.x as f64) as f32;
            let sy = args["scale_y"].as_f64().unwrap_or(old_scale.y as f64) as f32;
            let sz = args["scale_z"].as_f64().unwrap_or(old_scale.z as f64) as f32;

            let mut ur = get_undo_redo();
            ur.create_action(&format!("Set 3D Transform for {}", p));
            ur.add_do_property(
                &n.clone(),
                &StringName::from("position"),
                &Variant::from(godot::builtin::Vector3::new(px, py, pz)),
            );
            ur.add_do_property(
                &n.clone(),
                &StringName::from("rotation_degrees"),
                &Variant::from(godot::builtin::Vector3::new(rx, ry, rz)),
            );
            ur.add_do_property(
                &n.clone(),
                &StringName::from("scale"),
                &Variant::from(godot::builtin::Vector3::new(sx, sy, sz)),
            );
            ur.add_undo_property(
                &n.clone(),
                &StringName::from("position"),
                &Variant::from(old_pos),
            );
            ur.add_undo_property(
                &n.clone(),
                &StringName::from("rotation_degrees"),
                &Variant::from(old_rot),
            );
            ur.add_undo_property(
                &n.clone(),
                &StringName::from("scale"),
                &Variant::from(old_scale),
            );
            ur.commit_action();

            json!({"node_path": p, "x": px, "y": py, "z": pz, "rot_x": rx, "rot_y": ry, "rot_z": rz, "scale_x": sx, "scale_y": sy, "scale_z": sz})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_get_node_info(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let script_var = n.get("script");
            let script_path = script_var
                .try_to::<godot::obj::Gd<godot::classes::Resource>>()
                .ok()
                .map(|r| r.get_path().to_string())
                .unwrap_or_default();
            let visible: bool = n.get("visible").try_to().unwrap_or(true);
            let groups: Vec<String> = n
                .get_groups()
                .iter_shared()
                .map(|g| g.to_string())
                .collect();
            let unique = n.is_unique_name_in_owner();
            let sf = n.get_scene_file_path().to_string();
            let owner = n
                .get_owner()
                .map(|o| relative_path(&o, &root))
                .unwrap_or_default();
            let child_count = n.get_child_count();
            json!({
                "name": n.get_name().to_string(),
                "type": n.get_class().to_string(),
                "path": relative_path(&n, &root),
                "script": if script_path.is_empty() { Value::Null } else { json!(script_path) },
                "visible": visible,
                "groups": groups,
                "child_count": child_count,
                "owner": if owner.is_empty() { Value::Null } else { json!(owner) },
                "unique_name": unique,
                "is_instance": !sf.is_empty(),
                "scene_file_path": if sf.is_empty() { Value::Null } else { json!(sf) },
            })
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_get_script_variables(args: &Value) -> Value {
    let p = s(args, "node_path");
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let list: Vec<Value> = n
                .get_property_list()
                .iter_shared()
                .filter_map(|d| {
                    let usage: i64 = d.get(&Variant::from("usage"))?.try_to().unwrap_or(0);
                    // Align with scene_debugger_object.cpp: filter by SCRIPT_VARIABLE (0x10000000)
                    // and EDITOR (0x00000002), matching how Godot identifies exported variables.
                    if usage & 0x10000000 == 0 && usage & 0x00000002 == 0 {
                        return None;
                    }
                    let name = d.get(&Variant::from("name"))?.to_string();
                    let val = n.get(&StringName::from(name.as_str()));
                    let type_id: i64 = d.get(&Variant::from("type"))?.try_to().unwrap_or(0);
                    Some(json!({
                        "name": name,
                        "type": variant_type_name(type_id),
                        "value": v2j(&val),
                    }))
                })
                .collect();
            json!({"node_path": p, "variables": list})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

// ── Tree serialization ───────────────────────────────────────────────

fn serialize_tree(
    n: &godot::prelude::Gd<Node>,
    root: &godot::prelude::Gd<Node>,
    depth: i32,
    max: i32,
) -> Value {
    if depth > max {
        return json!({"name": "...", "type": "truncated"});
    }
    let visible: bool = n.get("visible").try_to().unwrap_or(true);
    let script_var = n.get("script");
    let script_path = script_var
        .try_to::<godot::obj::Gd<godot::classes::Resource>>()
        .ok()
        .map(|r| r.get_path().to_string())
        .unwrap_or_default();
    let children: Vec<Value> = (0..n.get_child_count())
        .filter_map(|i| {
            n.get_child(i)
                .map(|c| serialize_tree(&c, root, depth + 1, max))
        })
        .collect();
    json!({
        "name": n.get_name().to_string(),
        "type": n.get_class().to_string(),
        "path": relative_path(n, root),
        "visible": visible,
        "child_count": children.len(),
        "script": if script_path.is_empty() { Value::Null } else { json!(script_path) },
        "children": children,
    })
}
