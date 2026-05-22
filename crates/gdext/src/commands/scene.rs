use serde_json::{Value, json};

use godot::classes::{DirAccess, EditorInterface, FileAccess, Node, PackedScene};
use godot::obj::{NewAlloc, NewGd, Singleton};
use godot::prelude::{GString, StringName};
use godot::tools::{try_load, try_save};

use super::{CommandHandler, pipe, resolve_node, s};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "create_scene",
    "delete_scene",
    "rename_scene",
    "branch_to_scene",
    "scene_to_branch",
    "instantiate_scene",
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
        let a = args.clone();
        match tool {
            "create_scene" => pipe(d.submit(move || cmd_create_scene(&a)).await),
            "delete_scene" => pipe(d.submit(move || cmd_delete_scene(&a)).await),
            "rename_scene" => pipe(d.submit(move || cmd_rename_scene(&a)).await),
            "branch_to_scene" => pipe(d.submit(move || cmd_branch_to_scene(&a)).await),
            "scene_to_branch" => pipe(d.submit(move || cmd_scene_to_branch(&a)).await),
            "instantiate_scene" => pipe(d.submit(move || cmd_instantiate_scene(&a)).await),
            "open_scene" => pipe(d.submit(move || cmd_open_scene(&a)).await),
            "close_scene" => pipe(d.submit(move || cmd_close_scene(&a)).await),
            "save_scene" => pipe(d.submit(move || cmd_save_scene(&a)).await),
            "save_scene_as" => pipe(d.submit(move || cmd_save_scene_as(&a)).await),
            "save_all_scenes" => pipe(d.submit(move || cmd_save_all_scenes(&a)).await),
            "reload_scene" => pipe(d.submit(move || cmd_reload_scene(&a)).await),
            "get_open_scenes" => pipe(d.submit(move || cmd_get_open_scenes(&a)).await),
            "get_open_scene_roots" => pipe(d.submit(move || cmd_get_open_scene_roots(&a)).await),
            "mark_scene_unsaved" => pipe(d.submit(move || cmd_mark_scene_unsaved(&a)).await),
            _ => Err(format!("Unknown scene tool: {}", tool)),
        }
    }
}

// ── Scene file ───────────────────────────────────────────────────────

fn cmd_create_scene(args: &Value) -> Value {
    let p = s(args, "path");
    if p.is_empty() {
        return json!({"error": "missing 'path'"});
    }
    if !p.ends_with(".tscn") {
        return json!({"error": format!("path must end with .tscn: {}", p)});
    }
    if FileAccess::file_exists(&GString::from(&p)) {
        return json!({"error": format!("File already exists: {}. Use a different path or delete it first.", p)});
    }
    if let Some(slash) = p.rfind('/')
        && !DirAccess::dir_exists_absolute(&GString::from(&p[..slash]))
        && let Some(mut d) = DirAccess::open(&GString::from("res://"))
    {
        let dir = &p[..slash];
        let sub = dir.strip_prefix("res://").unwrap_or(dir);
        d.make_dir_recursive(&GString::from(sub));
    }
    let mut root = Node::new_alloc();
    root.set_name(&StringName::from("Root"));
    let mut packed = PackedScene::new_gd();
    packed.pack(&root);
    match try_save(&packed, p.as_str()) {
        Ok(()) => {
            root.free();
            if let Some(mut efs) = EditorInterface::singleton().get_resource_filesystem() {
                efs.update_file(&GString::from(&p));
            }
            json!({"path": p, "created": true})
        }
        Err(e) => {
            root.free();
            json!({"error": format!("Failed to save scene '{}': {}", p, e)})
        }
    }
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

// ── Editor scene tabs (open/close/save/reload/list) ─────────────────

fn cmd_open_scene(args: &Value) -> Value {
    let sp = s(args, "scene_path");
    if sp.is_empty() {
        return json!({"error": "missing 'scene_path'"});
    }
    if !FileAccess::file_exists(&GString::from(&sp)) {
        return json!({"error": format!("Scene file does not exist: {}", sp)});
    }
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
