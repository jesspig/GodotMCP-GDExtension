use serde_json::{Value, json};

use godot::classes::{ClassDb, DirAccess, EditorInterface, FileAccess, Node, PackedScene};
use godot::meta::ToGodot;
use godot::obj::{NewGd, Singleton};
use godot::prelude::{GString, StringName, Variant};
use godot::tools::{try_load, try_save};

use super::{
    CommandHandler, ReplaceOwnerMode, fix_owners_recursive, get_undo_redo, node_replace_owner,
    pipe, relative_path, resolve_node, s,
};
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
    "is_scene_dirty",
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
    fn handle<'a>(
        &'a self,
        tool: &'a str,
        args: &'a Value,
        d: &'a MainThreadDispatcher,
    ) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Value, String>> + Send + 'a>>
    {
        Box::pin(self.handle_scene_tool(tool, args, d))
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
            "is_scene_dirty" => pipe(d.submit(move || cmd_is_scene_dirty(&a)).await),
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
    super::ensure_parent_dir(&p);
    let root_type = s(args, "root_type");
    let root_type = if root_type.is_empty() {
        "Node"
    } else {
        &root_type
    };
    let root_name = s(args, "root_name");
    let root_name = if root_name.is_empty() {
        "Root"
    } else {
        &root_name
    };

    let variant = ClassDb::singleton().instantiate(&StringName::from(root_type));
    let mut root = match variant.try_to::<godot::prelude::Gd<Node>>() {
        Ok(n) => n,
        Err(_) => return json!({"error": format!("Cannot instantiate root type: {}", root_type)}),
    };
    root.set_name(&StringName::from(root_name));
    let mut packed = PackedScene::new_gd();
    packed.pack(&root);
    match try_save(&packed, p.as_str()) {
        Ok(()) => {
            root.free();
            if let Some(mut efs) = EditorInterface::singleton().get_resource_filesystem() {
                efs.update_file(&GString::from(&p));
            }
            json!({"path": p, "created": true, "root_type": root_type, "root_name": root_name})
        }
        Err(e) => {
            root.free();
            json!({"error": format!("Failed to save scene '{}': {}", p, e)})
        }
    }
}

fn cmd_delete_scene(args: &Value) -> Value {
    let p = s(args, "path");
    if p.is_empty() {
        return json!({"error": "missing 'path'"});
    }

    let mut ei = EditorInterface::singleton();
    let open_scenes = ei.get_open_scenes();
    let is_open =
        (0..open_scenes.len()).any(|i| open_scenes.get(i).is_some_and(|s| s.to_string() == p));

    if is_open {
        let is_current = ei
            .get_edited_scene_root()
            .is_some_and(|r| r.get_scene_file_path().to_string() == p);
        if !is_current {
            ei.open_scene_from_path_ex(&GString::from(p.as_str()))
                .done();
        }
        let close_err = ei.close_scene();
        if close_err != godot::global::Error::OK {
            return json!({"error": format!(
                "Failed to close scene '{}' before deletion: {:?}",
                p, close_err
            )});
        }
    }

    match DirAccess::open(&GString::from("res://")) {
        Some(mut d) => {
            let err = d.remove(&GString::from(p.as_str()));
            if err == godot::global::Error::OK {
                if let Some(mut efs) = ei.get_resource_filesystem() {
                    efs.update_file(&GString::from(p.as_str()));
                }
                json!({"deleted": p, "was_open": is_open})
            } else {
                json!({"error": format!("Failed to delete '{}': {:?}", p, err)})
            }
        }
        None => json!({"error": "Cannot open res://"}),
    }
}

fn cmd_rename_scene(args: &Value) -> Value {
    let src = s(args, "source_path");
    let dst = s(args, "dest_path");

    let mut ei = EditorInterface::singleton();
    let open_scenes = ei.get_open_scenes();
    let is_open =
        (0..open_scenes.len()).any(|i| open_scenes.get(i).is_some_and(|s| s.to_string() == src));

    if is_open {
        let scene_is_current = ei
            .get_edited_scene_root()
            .is_some_and(|r| r.get_scene_file_path().to_string() == src);
        if !scene_is_current {
            return json!({"error": format!(
                "Scene '{}' is open but not the active tab. Close it manually first, or make it active before renaming.", src
            )});
        }
        let _ = ei.save_scene();
        let close_err = ei.close_scene();
        if close_err != godot::global::Error::OK {
            return json!({"error": format!("Failed to close scene before rename: {:?}", close_err)});
        }
        match DirAccess::open(&GString::from("res://")) {
            Some(mut d) => {
                let err = d.rename(&GString::from(src.as_str()), &GString::from(dst.as_str()));
                if err != godot::global::Error::OK {
                    return json!({"error": format!("Failed to rename: {:?}", err)});
                }
            }
            None => return json!({"error": "Cannot open res://"}),
        }
        if let Some(mut efs) = ei.get_resource_filesystem() {
            efs.update_file(&GString::from(dst.as_str()));
        }
        ei.open_scene_from_path_ex(&GString::from(dst.as_str()))
            .done();
        json!({"source": src, "destination": dst, "tab_reopened": true})
    } else {
        match DirAccess::open(&GString::from("res://")) {
            Some(mut d) => {
                let err = d.rename(&GString::from(src.as_str()), &GString::from(dst.as_str()));
                if err == godot::global::Error::OK {
                    if let Some(mut efs) = ei.get_resource_filesystem() {
                        efs.update_file(&GString::from(dst.as_str()));
                    }
                    json!({"source": src, "destination": dst})
                } else {
                    json!({"error": format!("Failed to rename: {:?}", err)})
                }
            }
            None => json!({"error": "Cannot open res://"}),
        }
    }
}

fn cmd_branch_to_scene(args: &Value) -> Value {
    let p = s(args, "node_path");
    let sp = s(args, "scene_path");
    if sp.is_empty() {
        return json!({"error": "missing 'scene_path'"});
    }
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    let mut n = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };
    if n == root {
        return json!({"error": "Cannot branch the scene root node"});
    }

    let node_name = n.get_name().to_string();
    let parent = match n.get_parent() {
        Some(p) => p,
        None => return json!({"error": "Node has no parent"}),
    };
    let idx = n.get_index();

    let saved = match n.duplicate_node_ex().done_or_null() {
        Some(s) => s,
        None => return json!({"error": "Failed to duplicate node for undo backup"}),
    };

    fix_owners_recursive(&saved, &saved);

    let mut packed = PackedScene::new_gd();
    if packed.pack(&saved) != godot::global::Error::OK {
        return json!({"error": "Failed to pack scene"});
    }
    super::ensure_parent_dir(&sp);
    match try_save(&packed, sp.as_str()) {
        Ok(()) => {}
        Err(e) => return json!({"error": format!("Failed to save scene '{}': {}", sp, e)}),
    }
    if let Some(mut efs) = ei.get_resource_filesystem() {
        efs.update_file(&GString::from(&sp));
    }

    let inst = match try_load::<PackedScene>(&sp) {
        Ok(packed) => match packed.instantiate() {
            Some(i) => i,
            None => return json!({"error": "Failed to instantiate saved scene"}),
        },
        Err(e) => return json!({"error": format!("Failed to reload scene '{}': {}", sp, e)}),
    };
    let mut inst = inst;
    inst.set_name(&StringName::from(node_name.as_str()));

    let mut ur = get_undo_redo();
    ur.create_action(&format!("Branch to Scene: {}", sp));
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("remove_child"),
        &[inst.to_variant()],
    );
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("add_child"),
        &[saved.to_variant()],
    );
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("move_child"),
        &[saved.to_variant(), Variant::from(idx as i64)],
    );
    ur.add_undo_method(
        &saved.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    ur.add_undo_reference(&saved.clone());
    ur.commit_action_ex().execute(false).done();

    let mut parent = parent;
    parent.remove_child(&n);
    n.queue_free();
    parent.add_child(&inst);
    parent.move_child(&inst, idx);
    inst.set_owner(&root);

    json!({"scene_path": sp, "node_path": p, "replaced_with_instance": true})
}

fn cmd_scene_to_branch(args: &Value) -> Value {
    let p = s(args, "node_path");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    let n = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };
    let sf = n.get_scene_file_path();
    if sf.is_empty() {
        return json!({"error": format!(
            "Node '{}' is not an instanced scene (no scene_file_path). scene_to_branch only converts nodes that were instantiated from a .tscn file. Use branch_to_scene to save a regular node as a new .tscn.", p
        )});
    }
    let sf_str = sf.to_string();

    let mut ur = get_undo_redo();
    ur.create_action("Make Local");

    ur.add_do_method(
        &n.clone(),
        &StringName::from("set_scene_file_path"),
        &[Variant::from(GString::from(""))],
    );
    node_replace_owner(&n, &n, &root, &mut ur, ReplaceOwnerMode::Do);

    ur.add_undo_method(
        &n.clone(),
        &StringName::from("set_scene_file_path"),
        &[Variant::from(GString::from(sf_str.as_str()))],
    );
    node_replace_owner(&n, &n, &root, &mut ur, ReplaceOwnerMode::Undo);

    ur.commit_action();

    json!({"node_path": p, "converted": true, "source_scene": sf_str})
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
    let parent = match resolve_node(&root, pp.as_str()) {
        Some(p) => p,
        None => {
            return json!({"error": format!(
            "Parent not found: '{}'. To target the scene root, pass '', '.', '/root', or the root node's name.", pp)});
        }
    };
    let packed = match try_load::<PackedScene>(sp.as_str()) {
        Ok(p) => p,
        Err(e) => return json!({"error": format!("Scene '{}' could not be loaded: {}", sp, e)}),
    };
    let inst = match packed.instantiate() {
        Some(i) => i,
        None => return json!({"error": format!("Failed to instantiate scene from {}", sp)}),
    };
    let mut inst = inst;
    if let Some(ref n) = name
        && !n.is_empty()
    {
        inst.set_name(&StringName::from(n.as_str()));
    }

    let mut ur = get_undo_redo();
    ur.create_action(&format!("Instantiate Scene: {}", sp));
    ur.add_do_method(
        &parent.clone(),
        &StringName::from("add_child"),
        &[inst.to_variant()],
    );
    ur.add_do_method(
        &inst.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    ur.add_do_reference(&inst.clone());
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("remove_child"),
        &[inst.to_variant()],
    );
    ur.commit_action();

    json!({"path": relative_path(&inst, &root), "parent": relative_path(&parent, &root)})
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
    let root_after = ei.get_edited_scene_root();
    let loaded = root_after.is_some();

    let mut result = if loaded {
        json!({"opened": sp, "loaded": true})
    } else {
        json!({
            "opened": sp,
            "loaded": false,
            "error": "Scene file was opened but the scene root could not be loaded. \
                      This usually means the scene references resources that no longer exist. \
                      Check for broken ExtResource references in the .tscn file."
        })
    };

    if set_inherited {
        let open_scenes = ei.get_open_scenes();
        let len = open_scenes.len();
        let has_empty = (0..len).any(|i| {
            open_scenes
                .get(i)
                .unwrap_or_default()
                .to_string()
                .is_empty()
        });
        if has_empty {
            result
                .as_object_mut()
                .unwrap()
                .insert("warning".into(), json!("Godot 4.6 creates an empty scene tab when opening with set_inherited=true. Close the empty tab manually in the editor."));
        }
    }

    result
}

fn cmd_close_scene(_args: &Value) -> Value {
    let mut ei = EditorInterface::singleton();
    let before: Vec<String> = (0..ei.get_open_scenes().len())
        .map(|i| ei.get_open_scenes().get(i).unwrap_or_default().to_string())
        .collect();
    let err = ei.close_scene();
    if err == godot::global::Error::OK {
        let after: Vec<String> = (0..ei.get_open_scenes().len())
            .map(|i| ei.get_open_scenes().get(i).unwrap_or_default().to_string())
            .collect();
        let closed = before.iter().find(|p| !after.contains(p));
        json!({"closed": true, "scene": closed})
    } else {
        json!({"error": format!("close_scene failed: {:?}", err)})
    }
}

fn cmd_save_scene(_args: &Value) -> Value {
    let mut ei = EditorInterface::singleton();
    let root = ei.get_edited_scene_root();
    let warnings = root
        .as_ref()
        .map(collect_owner_warnings)
        .unwrap_or_default();
    let err = ei.save_scene();
    if err == godot::global::Error::OK {
        let path = root
            .as_ref()
            .map(|r| r.get_scene_file_path().to_string())
            .unwrap_or_default();
        if let Some(ref r) = root {
            save_version_marker(r, &ei);
        }
        let mut result = json!({"saved": path});
        if !warnings.is_empty() {
            result
                .as_object_mut()
                .unwrap()
                .insert("warning".into(), json!(warnings));
        }
        result
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
    let root = ei.get_edited_scene_root();
    let warnings = root
        .as_ref()
        .map(collect_owner_warnings)
        .unwrap_or_default();
    ei.save_scene_as_ex(&GString::from(sp.as_str()))
        .with_preview(with_preview)
        .done();
    if let Some(ref r) = root {
        save_version_marker(r, &ei);
    }
    let mut result = json!({"saved": sp});
    if !warnings.is_empty() {
        result
            .as_object_mut()
            .unwrap()
            .insert("warning".into(), json!(warnings));
    }
    result
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
            let scene_fp = r.get_scene_file_path().to_string();
            let display_path = if scene_fp.is_empty() {
                r.get_name().to_string()
            } else {
                scene_fp
            };
            json!({
                "name": r.get_name().to_string(),
                "class": r.get_class().to_string(),
                "scene_file_path": display_path,
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

fn cmd_is_scene_dirty(_args: &Value) -> Value {
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"dirty": false, "hint": "No scene open"}),
    };
    let ur = match ei.get_editor_undo_redo() {
        Some(ur) => ur,
        None => return json!({"dirty": false}),
    };
    let history_id = ur.get_object_history_id(&root);
    let dirty = match ur.get_history_undo_redo(history_id) {
        Some(undo_redo) => {
            let version = undo_redo.get_version();
            let saved_key = StringName::from("__mcp_saved_version");
            let saved_version: i64 = root.get_meta(&saved_key).try_to().unwrap_or(0);
            version > (saved_version as u64)
        }
        None => false,
    };
    json!({"dirty": dirty})
}

fn collect_owner_warnings(root: &godot::obj::Gd<Node>) -> Vec<String> {
    let mut orphaned: Vec<String> = Vec::new();
    let mut stack = vec![root.clone()];
    while let Some(node) = stack.pop() {
        let count = node.get_child_count();
        for i in 0..count {
            if let Some(child) = node.get_child(i) {
                if child.get_owner().is_none() && child != *root {
                    orphaned.push(format!("{} ({})", child.get_name(), child.get_class()));
                }
                stack.push(child);
            }
        }
    }
    if orphaned.is_empty() {
        return Vec::new();
    }
    vec![format!(
        "Scene has {} node(s) with owner=null that will be excluded from the saved file: {}",
        orphaned.len(),
        orphaned.join(", ")
    )]
}

fn save_version_marker(root: &godot::obj::Gd<Node>, ei: &godot::obj::Gd<EditorInterface>) {
    if let Some(ur) = ei.get_editor_undo_redo() {
        let history_id = ur.get_object_history_id(root);
        if let Some(undo_redo) = ur.get_history_undo_redo(history_id) {
            let version = undo_redo.get_version();
            let mut r = root.clone();
            r.set_meta(
                &godot::prelude::StringName::from("__mcp_saved_version"),
                &godot::prelude::Variant::from(version as i64),
            );
        }
    }
}
