use serde_json::{Value, json};

use godot::classes::{ClassDb, DirAccess, EditorInterface, Node, PackedScene};
use godot::obj::{NewAlloc, NewGd, Singleton};
use godot::prelude::{GString, StringName, Variant};
use godot::tools::try_load;

use super::{CommandHandler, j2v, pipe, resolve_node, s, v2j};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "create_scene",
    "delete_scene",
    "rename_scene",
    "get_scene_tree",
    "create_node",
    "delete_node",
    "move_node",
    "duplicate_node",
    "rename_node",
    "attach_script",
    "detach_script",
    "find_nodes",
    "branch_to_scene",
    "scene_to_branch",
    "reset_parent",
    "set_as_root",
    "get_node_path",
    "instantiate_scene",
    "get_property_list",
    "get_property",
    "set_property",
    "batch_set_property",
    "open_scene",
    "close_scene",
    "save_scene",
    "save_scene_as",
    "save_all_scenes",
    "reload_scene",
    "get_open_scenes",
    "get_open_scene_roots",
    "mark_scene_unsaved",
];

pub struct SceneCommands;

impl SceneCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for SceneCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for SceneCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn execute(&self, _args: &Value, _d: &MainThreadDispatcher) -> Result<Value, String> {
        Err("SceneCommands::execute should not be called directly".into())
    }
    fn group_name(&self) -> &str {
        "scene"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl SceneCommands {
    pub async fn handle_scene_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        match tool {
            "get_scene_tree" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_get_scene_tree(&a)).await)
            }
            "get_node_path" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_get_node_path(&a)).await)
            }
            "get_property_list" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_get_property_list(&a)).await)
            }
            "get_property" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_get_property(&a)).await)
            }
            "create_node" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_create_node(&a)).await)
            }
            "delete_node" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_delete_node(&a)).await)
            }
            "rename_node" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_rename_node(&a)).await)
            }
            "set_property" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_set_property(&a)).await)
            }
            "duplicate_node" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_duplicate_node(&a)).await)
            }
            "move_node" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_move_node(&a)).await)
            }
            "attach_script" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_attach_script(&a)).await)
            }
            "detach_script" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_detach_script(&a)).await)
            }
            "find_nodes" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_find_nodes(&a)).await)
            }
            "create_scene" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_create_scene(&a)).await)
            }
            "delete_scene" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_delete_scene(&a)).await)
            }
            "rename_scene" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_rename_scene(&a)).await)
            }
            "branch_to_scene" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_branch_to_scene(&a)).await)
            }
            "scene_to_branch" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_scene_to_branch(&a)).await)
            }
            "instantiate_scene" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_instantiate_scene(&a)).await)
            }
            "reset_parent" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_reset_parent(&a)).await)
            }
            "set_as_root" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_set_as_root(&a)).await)
            }
            "batch_set_property" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_batch_set_property(&a)).await)
            }
            "open_scene" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_open_scene(&a)).await)
            }
            "close_scene" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_close_scene(&a)).await)
            }
            "save_scene" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_save_scene(&a)).await)
            }
            "save_scene_as" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_save_scene_as(&a)).await)
            }
            "save_all_scenes" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_save_all_scenes(&a)).await)
            }
            "reload_scene" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_reload_scene(&a)).await)
            }
            "get_open_scenes" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_get_open_scenes(&a)).await)
            }
            "get_open_scene_roots" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_get_open_scene_roots(&a)).await)
            }
            "mark_scene_unsaved" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_mark_scene_unsaved(&a)).await)
            }
            _ => Err(format!("Unknown scene tool: {}", tool)),
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

// ── Script + search ──────────────────────────────────────────────────

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

// ── Scene file ───────────────────────────────────────────────────────

fn cmd_create_scene(args: &Value) -> Value {
    let p = s(args, "path");
    let mut root = Node::new_alloc();
    root.set_name(&StringName::from("Root"));
    let mut packed = PackedScene::new_gd();
    packed.pack(&root);
    godot::tools::save(&packed, p.as_str());
    root.free();
    json!({"path": p})
}

fn cmd_delete_scene(args: &Value) -> Value {
    let p = s(args, "path");
    match DirAccess::open(&GString::from("res://")) {
        Some(mut d) => {
            let err = d.remove(&GString::from(p.as_str()));
            if err == godot::global::Error::OK {
                json!({"deleted": p})
            } else {
                json!({"error": format!("Failed to delete: {:?}", err)})
            }
        }
        None => json!({"error": "Cannot open res://"}),
    }
}

fn cmd_rename_scene(args: &Value) -> Value {
    let src = s(args, "source_path");
    let dst = s(args, "dest_path");
    match DirAccess::open(&GString::from("res://")) {
        Some(mut d) => {
            let err = d.rename(&GString::from(src.as_str()), &GString::from(dst.as_str()));
            if err == godot::global::Error::OK {
                json!({"source": src, "destination": dst})
            } else {
                json!({"error": format!("Failed to rename: {:?}", err)})
            }
        }
        None => json!({"error": "Cannot open res://"}),
    }
}

fn cmd_branch_to_scene(args: &Value) -> Value {
    let p = s(args, "node_path");
    let sp = s(args, "scene_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let mut packed = PackedScene::new_gd();
            packed.pack(&n);
            godot::tools::save(&packed, sp.as_str());
            json!({"scene_path": sp, "node_path": p})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_scene_to_branch(args: &Value) -> Value {
    let p = s(args, "node_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(n) => {
            let sf = n.get_scene_file_path();
            if sf.is_empty() {
                return json!({"error": format!(
                    "Node '{}' is not an instanced scene (no scene_file_path). scene_to_branch only converts nodes that were instantiated from a .tscn file via instantiate_scene or PackedScene.instantiate(). Use branch_to_scene to save a regular node as a new .tscn.", p)});
            }
            match try_load::<PackedScene>(sf.to_string().as_str()) {
                Ok(packed) => match packed.instantiate() {
                    Some(mut inst) => {
                        let mut parent = n.get_parent().unwrap();
                        let idx = n.get_index();
                        parent.remove_child(&n);
                        parent.add_child(&inst);
                        parent.move_child(&inst, idx);
                        inst.set_owner(&root);
                        json!({"node_path": p, "converted": true, "source_scene": sf.to_string()})
                    }
                    None => {
                        json!({"error": format!("Failed to instantiate scene from {}", sf.to_string())})
                    }
                },
                Err(e) => {
                    json!({"error": format!("Source scene '{}' could not be loaded: {}", sf.to_string(), e)})
                }
            }
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}

fn cmd_instantiate_scene(args: &Value) -> Value {
    let sp = s(args, "scene_path");
    let pp = s(args, "parent_path");
    let name = args["name"].as_str().map(|v| v.to_string());
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    let mut parent = match resolve_node(&root, pp.as_str()) {
        Some(p) => p,
        None => {
            return json!({"error": format!(
            "Parent not found: '{}'. To target the scene root, pass '', '.', '/root', or the root node's name.", pp)});
        }
    };
    match try_load::<PackedScene>(sp.as_str()) {
        Ok(packed) => match packed.instantiate() {
            Some(mut inst) => {
                if let Some(n) = name {
                    inst.set_name(&StringName::from(n.as_str()));
                }
                parent.add_child(&inst);
                inst.set_owner(&root);
                json!({"path": inst.get_path().to_string(), "parent": parent.get_path().to_string()})
            }
            None => json!({"error": format!("Failed to instantiate scene from {}", sp)}),
        },
        Err(e) => json!({"error": format!("Scene '{}' could not be loaded: {}", sp, e)}),
    }
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

// ── Editor scene tabs (open/close/save/reload/list) ─────────────────

fn cmd_open_scene(args: &Value) -> Value {
    let sp = s(args, "scene_path");
    let set_inherited = args["set_inherited"].as_bool().unwrap_or(false);
    let mut ei = EditorInterface::singleton();
    ei.open_scene_from_path_ex(&GString::from(sp.as_str()))
        .set_inherited(set_inherited)
        .done();
    json!({"opened": sp})
}

fn cmd_close_scene(_args: &Value) -> Value {
    let mut ei = EditorInterface::singleton();
    let err = ei.close_scene();
    if err == godot::global::Error::OK {
        json!({"closed": true})
    } else {
        json!({"error": format!("close_scene failed: {:?}", err)})
    }
}

fn cmd_save_scene(_args: &Value) -> Value {
    let mut ei = EditorInterface::singleton();
    let err = ei.save_scene();
    if err == godot::global::Error::OK {
        let path = ei
            .get_edited_scene_root()
            .map(|r| r.get_scene_file_path().to_string())
            .unwrap_or_default();
        json!({"saved": path})
    } else {
        json!({"error": format!("save_scene failed: {:?}. The scene may not have a file path yet — use save_scene_as first.", err)})
    }
}

fn cmd_save_scene_as(args: &Value) -> Value {
    let sp = s(args, "scene_path");
    if sp.is_empty() {
        return json!({"error": "missing 'scene_path'"});
    }
    let with_preview = args["with_preview"].as_bool().unwrap_or(true);
    let mut ei = EditorInterface::singleton();
    ei.save_scene_as_ex(&GString::from(sp.as_str()))
        .with_preview(with_preview)
        .done();
    json!({"saved": sp})
}

fn cmd_save_all_scenes(_args: &Value) -> Value {
    let mut ei = EditorInterface::singleton();
    let before = ei.get_open_scenes().len();
    ei.save_all_scenes();
    json!({"count": before})
}

fn cmd_reload_scene(args: &Value) -> Value {
    let sp = s(args, "scene_path");
    if sp.is_empty() {
        return json!({"error": "missing 'scene_path'"});
    }
    let mut ei = EditorInterface::singleton();
    ei.reload_scene_from_path(&GString::from(sp.as_str()));
    json!({"reloaded": sp})
}

fn cmd_get_open_scenes(_args: &Value) -> Value {
    let ei = EditorInterface::singleton();
    let scenes = ei.get_open_scenes();
    let list: Vec<String> = (0..scenes.len())
        .map(|i| scenes.get(i).unwrap_or_default().to_string())
        .collect();
    json!({"scenes": list})
}

fn cmd_get_open_scene_roots(_args: &Value) -> Value {
    let ei = EditorInterface::singleton();
    let roots = ei.get_open_scene_roots();
    let list: Vec<Value> = roots
        .iter_shared()
        .map(|r| {
            json!({
                "name": r.get_name().to_string(),
                "path": r.get_path().to_string(),
                "scene_file_path": r.get_scene_file_path().to_string(),
                "class": r.get_class().to_string(),
            })
        })
        .collect();
    json!({"roots": list})
}

fn cmd_mark_scene_unsaved(_args: &Value) -> Value {
    let mut ei = EditorInterface::singleton();
    ei.mark_scene_as_unsaved();
    json!({"marked": true})
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
