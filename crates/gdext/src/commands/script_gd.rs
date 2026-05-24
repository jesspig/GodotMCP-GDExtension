use serde_json::{Value, json};

use godot::classes::file_access::ModeFlags;
use godot::classes::{DirAccess, EditorInterface, FileAccess, GDScript, ProjectSettings};
use godot::obj::Singleton;
use godot::prelude::GString;

use super::{CommandHandler, s};
use crate::dispatcher::MainThreadDispatcher;
use crate::lsp::validate_via_lsp;

const DEFAULT_BASE_CLASS: &str = "RefCounted";

pub const TOOL_NAMES: &[&str] = &[
    "create_gdscript",
    "read_gdscript",
    "edit_gdscript",
    "validate_gdscript",
    "list_gdscripts",
];

pub struct ScriptGdCommands;

impl ScriptGdCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for ScriptGdCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for ScriptGdCommands {
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
        Box::pin(self.handle_script_gd_tool(tool, args, d))
    }
    fn group_name(&self) -> &str {
        "script_gd"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl ScriptGdCommands {
    pub async fn handle_script_gd_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "create_gdscript" => super::pipe(d.submit(move || cmd_create_gdscript(&a)).await),
            "read_gdscript" => super::pipe(d.submit(move || cmd_read_gdscript(&a)).await),
            "edit_gdscript" => super::pipe(d.submit(move || cmd_edit_gdscript(&a)).await),
            "validate_gdscript" => super::pipe(d.submit(move || cmd_validate_gdscript(&a)).await),
            "list_gdscripts" => super::pipe(d.submit(move || cmd_list_gdscripts(&a)).await),
            _ => Err(format!("Unknown GDScript tool: {}", tool)),
        }
    }
}

fn cmd_create_gdscript(args: &Value) -> Value {
    let path = s(args, "path");
    let base_class = s(args, "base_class");
    let base_class = if base_class.is_empty() {
        DEFAULT_BASE_CLASS.to_string()
    } else {
        base_class
    };
    let class_name = s(args, "class_name");
    let template = s(args, "template");
    let overwrite = args["overwrite"].as_bool().unwrap_or(false);

    if path.is_empty() {
        return json!({"error": "missing 'path'"});
    }
    if !path.ends_with(".gd") {
        return json!({"error": format!("path must end with .gd: {}", path)});
    }
    if FileAccess::file_exists(&GString::from(&path)) && !overwrite {
        return json!({"error": format!("File already exists: {}. Set overwrite=true to replace.", path)});
    }

    let parent_dir = path.rfind('/').map(|i| &path[..i]).unwrap_or("res://");
    if !DirAccess::dir_exists_absolute(&GString::from(parent_dir)) {
        super::ensure_parent_dir(&path);
    }

    let ts = "\t";
    let body = if !template.is_empty() {
        match template.as_str() {
            "empty" => {
                let cn = if class_name.is_empty() {
                    String::new()
                } else {
                    format!("class_name {}\n", class_name)
                };
                format!("extends {}\n{}", base_class, cn)
            }
            other => other
                .replace("_BASE_", &base_class)
                .replace("_CLASS_", &class_name)
                .replace("_TS_", ts),
        }
    } else {
        let cn = if class_name.is_empty() {
            String::new()
        } else {
            format!("class_name {}\n", class_name)
        };
        format!(
            "extends {}\n{}# Called when the node enters the scene tree for the first time.\nfunc _ready() -> void:\n{}pass\n\n# Called every frame. 'delta' is the elapsed time since the previous frame.\nfunc _process(delta: float) -> void:\n{}pass\n",
            base_class, cn, ts, ts
        )
    };

    match FileAccess::open(&GString::from(&path), ModeFlags::WRITE) {
        Some(mut f) => {
            f.store_string(&GString::from(&body));
            let bytes = body.len() as i64;
            if let Some(mut ei) = EditorInterface::singleton().get_resource_filesystem() {
                ei.update_file(&GString::from(&path));
            }
            json!({"path": path, "created": true, "bytes": bytes, "language": "gdscript"})
        }
        None => json!({"error": format!("Cannot open file for writing: {}", path)}),
    }
}

fn cmd_read_gdscript(args: &Value) -> Value {
    let path = s(args, "path");
    if !FileAccess::file_exists(&GString::from(&path)) {
        return json!({"error": format!("File not found: {}", path)});
    }
    let from_editor = args["from_editor"].as_bool().unwrap_or(false);
    if from_editor {
        let disk_content = FileAccess::get_file_as_string(&GString::from(&path)).to_string();
        if let Ok(script) = godot::tools::try_load::<GDScript>(&path) {
            let editor_source = script.get_source_code().to_string();
            if editor_source.len() != disk_content.len() {
                return json!({
                    "path": path,
                    "source": disk_content,
                    "length": disk_content.len(),
                    "language": "gdscript",
                    "stale_cache": true,
                    "cached_length": editor_source.len()
                });
            }
            return json!({"path": path, "source": editor_source, "length": editor_source.len(), "language": "gdscript"});
        }
    }
    let content = FileAccess::get_file_as_string(&GString::from(&path)).to_string();
    json!({"path": path, "source": content, "length": content.len(), "language": "gdscript"})
}

fn cmd_edit_gdscript(args: &Value) -> Value {
    let path = s(args, "path");
    let source = s(args, "source");
    if !FileAccess::file_exists(&GString::from(&path)) {
        return json!({"error": format!("File not found: {}. Use create_gdscript to create a new file.", path)});
    }
    if source.is_empty() {
        return json!({"error": "missing 'source'"});
    }
    match FileAccess::open(&GString::from(&path), ModeFlags::WRITE) {
        Some(mut f) => {
            f.store_string(&GString::from(&source));
            let bytes = source.len() as i64;
            if let Some(mut efs) = EditorInterface::singleton().get_resource_filesystem() {
                efs.update_file(&GString::from(&path));
            }
            let mut reload_error: Option<String> = None;
            if let Ok(mut script) = godot::tools::try_load::<GDScript>(&path) {
                script.set_source_code(&GString::from(&source));
                let mut err = script.reload();
                if err != godot::global::Error::OK {
                    err = script.reload();
                }
                if err != godot::global::Error::OK {
                    reload_error = Some(format!("{:?}", err));
                } else {
                    let mut ei = EditorInterface::singleton();
                    ei.edit_script_ex(&script).done();
                }
            }
            let mut result =
                json!({"path": path, "bytes": bytes, "reloaded": reload_error.is_none()});
            if let Some(e) = reload_error {
                result["reload_error"] = json!(e);
            }
            result
        }
        None => json!({"error": format!("Cannot open file for writing: {}", path)}),
    }
}

fn cmd_validate_gdscript(args: &Value) -> Value {
    let path = s(args, "path");
    if !FileAccess::file_exists(&GString::from(&path)) {
        return json!({"error": format!("File not found: {}", path)});
    }
    let source = FileAccess::get_file_as_string(&GString::from(&path)).to_string();
    let project_root = ProjectSettings::singleton()
        .globalize_path(&GString::from("res://"))
        .to_string();
    let file_abs = ProjectSettings::singleton()
        .globalize_path(&GString::from(&path))
        .to_string();
    let file_uri = format!("file:///{}", file_abs.replace('\\', "/"));
    let root_uri = format!("file:///{}", project_root.replace('\\', "/"));

    let port = EditorInterface::singleton()
        .get_editor_settings()
        .and_then(|s| {
            s.get_setting(&GString::from("network/language_server/remote_port"))
                .try_to::<i64>()
                .ok()
        })
        .unwrap_or(6005) as u16;

    let rt = match tokio::runtime::Runtime::new() {
        Ok(r) => r,
        Err(e) => {
            return json!({"error": format!("Cannot create tokio runtime for LSP: {}", e)});
        }
    };
    let result = rt.block_on(validate_via_lsp(port, &path, &file_uri, &source, &root_uri));

    match result {
        Ok(diagnostics) => {
            let diag_json: Vec<Value> = diagnostics
                .iter()
                .map(|d| {
                    json!({
                        "line": d.range.start.line + 1,
                        "column": d.range.start.character + 1,
                        "severity": d.severity.unwrap_or(1),
                        "message": d.message,
                        "source": d.source
                    })
                })
                .collect();
            let ok = diag_json.is_empty();
            json!({"path": path, "ok": ok, "diagnostics": diag_json})
        }
        Err(e) => json!({"error": format!(
            "{}. Ensure the GDScript Language Server is enabled in Editor Settings > Network > Language Server > Enable.", e
        )}),
    }
}

fn cmd_list_gdscripts(args: &Value) -> Value {
    let root = {
        let r = s(args, "root");
        if r.is_empty() {
            "res://".to_string()
        } else {
            r
        }
    };
    let include_addons = args["include_addons"].as_bool().unwrap_or(false);
    let max_results = args["max_results"].as_u64().unwrap_or(1000) as usize;

    let mut files = Vec::new();
    list_scripts_recursive(&root, "gd", include_addons, max_results, &mut files);

    let truncated = files.len() >= max_results;
    let scripts: Vec<Value> = files
        .iter()
        .map(|f| json!({"path": f.path, "size": f.size}))
        .collect();
    json!({"scripts": scripts, "count": scripts.len(), "truncated": truncated})
}

struct ScriptEntry {
    path: String,
    size: i64,
}

fn list_scripts_recursive(
    dir: &str,
    ext: &str,
    include_addons: bool,
    max: usize,
    out: &mut Vec<ScriptEntry>,
) {
    if out.len() >= max {
        return;
    }
    let Some(mut d) = DirAccess::open(&GString::from(dir)) else {
        return;
    };
    d.list_dir_begin();
    loop {
        let name = d.get_next().to_string();
        if name.is_empty() {
            break;
        }
        if name == "." || name == ".." {
            continue;
        }
        let full = if dir == "res://" {
            format!("res://{}", name)
        } else {
            format!("{}/{}", dir, name)
        };
        if d.current_is_dir() {
            if !include_addons && (name == "addons" || name == ".godot" || name == ".import") {
                continue;
            }
            list_scripts_recursive(&full, ext, include_addons, max, out);
        } else if name.ends_with(&format!(".{}", ext)) && out.len() < max {
            let sz = FileAccess::get_file_as_bytes(&GString::from(&full)).len() as i64;
            out.push(ScriptEntry {
                path: full,
                size: sz,
            });
        }
    }
}
