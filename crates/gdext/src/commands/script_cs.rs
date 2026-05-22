use serde_json::{Value, json};

use godot::classes::file_access::ModeFlags;
use godot::classes::{DirAccess, EditorInterface, FileAccess};
use godot::obj::Singleton;
use godot::prelude::GString;

use super::{CommandHandler, globalize_path, s};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "create_csharp_script",
    "read_csharp_script",
    "edit_csharp_script",
    "list_csharp_scripts",
    "csharp_build",
    "csharp_create_solution",
];

pub struct ScriptCsCommands;

impl ScriptCsCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for ScriptCsCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for ScriptCsCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn execute(&self, _args: &Value, _d: &MainThreadDispatcher) -> Result<Value, String> {
        Err("ScriptCsCommands::execute should not be called directly".into())
    }
    fn group_name(&self) -> &str {
        "script_cs"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl ScriptCsCommands {
    pub async fn handle_script_cs_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "create_csharp_script" => {
                super::pipe(d.submit(move || cmd_create_csharp_script(&a)).await)
            }
            "read_csharp_script" => super::pipe(d.submit(move || cmd_read_csharp_script(&a)).await),
            "edit_csharp_script" => super::pipe(d.submit(move || cmd_edit_csharp_script(&a)).await),
            "list_csharp_scripts" => {
                super::pipe(d.submit(move || cmd_list_csharp_scripts(&a)).await)
            }
            "csharp_build" => super::pipe(d.submit(move || cmd_csharp_build(&a)).await),
            "csharp_create_solution" => {
                super::pipe(d.submit(move || cmd_csharp_create_solution(&a)).await)
            }
            _ => Err(format!("Unknown C# tool: {}", tool)),
        }
    }
}

fn class_name_from_path(path: &str) -> String {
    let file = path.rsplit('/').next().unwrap_or(path);
    let stem = file.strip_suffix(".cs").unwrap_or(file);
    stem.to_string()
}

fn find_csproj() -> Option<String> {
    let root = "res://";
    let mut d = DirAccess::open(&GString::from(root))?;
    d.list_dir_begin();
    loop {
        let name = d.get_next().to_string();
        if name.is_empty() {
            break;
        }
        if name == "." || name == ".." {
            continue;
        }
        if !d.current_is_dir() && name.ends_with(".csproj") {
            return Some(format!("res://{}", name));
        }
    }
    None
}

fn cmd_create_csharp_script(args: &Value) -> Value {
    let path = s(args, "path");
    let base_class = s(args, "base_class");
    let base_class = if base_class.is_empty() {
        "Node".to_string()
    } else {
        base_class
    };
    let overwrite = args["overwrite"].as_bool().unwrap_or(false);

    if path.is_empty() {
        return json!({"error": "missing 'path'"});
    }
    if !path.ends_with(".cs") {
        return json!({"error": format!("path must end with .cs: {}", path)});
    }
    if find_csproj().is_none() {
        return json!({"error": "No .csproj found in project root. Call csharp_create_solution first to initialize the C# project."});
    }
    if FileAccess::file_exists(&GString::from(&path)) && !overwrite {
        return json!({"error": format!("File already exists: {}. Set overwrite=true to replace.", path)});
    }

    let parent_dir = path.rfind('/').map(|i| &path[..i]).unwrap_or("res://");
    if !DirAccess::dir_exists_absolute(&GString::from(parent_dir))
        && let Some(mut d) = DirAccess::open(&GString::from("res://"))
    {
        let sub = parent_dir.strip_prefix("res://").unwrap_or(parent_dir);
        d.make_dir_recursive(&GString::from(sub));
    }

    let class_name = class_name_from_path(&path);
    let body = format!(
        "using Godot;\nusing System;\n\npublic partial class {} : {}\n{{\n    public override void _Ready()\n    {{\n        base._Ready();\n    }}\n\n    public override void _Process(double delta)\n    {{\n        base._Process(delta);\n    }}\n}}\n",
        class_name, base_class
    );

    match FileAccess::open(&GString::from(&path), ModeFlags::WRITE) {
        Some(mut f) => {
            f.store_string(&GString::from(&body));
            let bytes = body.len() as i64;
            if let Some(mut efs) = EditorInterface::singleton().get_resource_filesystem() {
                efs.update_file(&GString::from(&path));
            }
            json!({"path": path, "created": true, "bytes": bytes, "language": "csharp", "class_name": class_name})
        }
        None => json!({"error": format!("Cannot open file for writing: {}", path)}),
    }
}

fn cmd_read_csharp_script(args: &Value) -> Value {
    let path = s(args, "path");
    if !FileAccess::file_exists(&GString::from(&path)) {
        return json!({"error": format!("File not found: {}", path)});
    }
    let content = FileAccess::get_file_as_string(&GString::from(&path)).to_string();
    json!({"path": path, "source": content, "length": content.len(), "language": "csharp"})
}

fn cmd_edit_csharp_script(args: &Value) -> Value {
    let path = s(args, "path");
    let source = s(args, "source");
    if !FileAccess::file_exists(&GString::from(&path)) {
        return json!({"error": format!("File not found: {}. Use create_csharp_script to create a new file.", path)});
    }
    if source.is_empty() {
        return json!({"error": "missing 'source'"});
    }
    match FileAccess::open(&GString::from(&path), ModeFlags::WRITE) {
        Some(mut f) => {
            f.store_string(&GString::from(&source));
            let bytes = source.len() as i64;
            let mut updated_file = false;
            if let Some(mut efs) = EditorInterface::singleton().get_resource_filesystem() {
                efs.update_file(&GString::from(&path));
                updated_file = true;
            }
            json!({"path": path, "bytes": bytes, "updated_file": updated_file, "language": "csharp", "note": "C# requires csharp_build to take effect"})
        }
        None => json!({"error": format!("Cannot open file for writing: {}", path)}),
    }
}

fn cmd_list_csharp_scripts(args: &Value) -> Value {
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
    list_cs_recursive(&root, include_addons, max_results, &mut files);

    let truncated = files.len() >= max_results;
    let scripts: Vec<Value> = files
        .iter()
        .map(|f| json!({"path": f.path, "size": f.size}))
        .collect();
    json!({"scripts": scripts, "count": scripts.len(), "truncated": truncated})
}

struct CsEntry {
    path: String,
    size: i64,
}

fn list_cs_recursive(dir: &str, include_addons: bool, max: usize, out: &mut Vec<CsEntry>) {
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
            list_cs_recursive(&full, include_addons, max, out);
        } else if name.ends_with(".cs") && out.len() < max {
            let sz = FileAccess::get_file_as_bytes(&GString::from(&full)).len() as i64;
            out.push(CsEntry {
                path: full,
                size: sz,
            });
        }
    }
}

fn parse_msbuild_errors(output: &str) -> Vec<Value> {
    let re = match regex::Regex::new(
        r"^(.+?)\((\d+),(\d+)\):\s+(error|warning)\s+([A-Z]+\d+):\s+(.+?)(?:\s+\[.*\])?$",
    ) {
        Ok(r) => r,
        Err(_) => return vec![],
    };
    let mut out = Vec::new();
    for line in output.lines() {
        if let Some(c) = re.captures(line.trim()) {
            out.push(json!({
                "file": c.get(1).map(|m| m.as_str()).unwrap_or(""),
                "line": c.get(2).and_then(|m| m.as_str().parse::<u64>().ok()).unwrap_or(0),
                "column": c.get(3).and_then(|m| m.as_str().parse::<u64>().ok()).unwrap_or(0),
                "severity": c.get(4).map(|m| m.as_str()).unwrap_or(""),
                "code": c.get(5).map(|m| m.as_str()).unwrap_or(""),
                "message": c.get(6).map(|m| m.as_str()).unwrap_or(""),
            }));
        }
    }
    out
}

fn cmd_csharp_build(args: &Value) -> Value {
    let configuration = {
        let c = s(args, "configuration");
        if c.is_empty() { "Debug".to_string() } else { c }
    };
    let project_root = match globalize_path("res://") {
        Some(p) => p,
        None => return json!({"error": "Cannot resolve project root"}),
    };

    let output = std::process::Command::new("dotnet")
        .args(["build", "--nologo", "--configuration", &configuration])
        .current_dir(&project_root)
        .output();

    match output {
        Ok(o) => {
            let stdout = String::from_utf8_lossy(&o.stdout).to_string();
            let stderr = String::from_utf8_lossy(&o.stderr).to_string();
            let combined = format!("{}\n{}", stdout, stderr);
            let errors = parse_msbuild_errors(&combined);
            json!({
                "exit_code": o.status.code(),
                "success": o.status.success(),
                "stdout": stdout,
                "stderr": stderr,
                "errors": errors,
                "project_root": project_root,
                "configuration": configuration
            })
        }
        Err(e) => json!({
            "error": format!("Failed to spawn dotnet: {}. Ensure .NET SDK is installed and on PATH.", e),
            "project_root": project_root
        }),
    }
}

fn cmd_csharp_create_solution(_args: &Value) -> Value {
    let project_root = match globalize_path("res://") {
        Some(p) => p,
        None => return json!({"error": "Cannot resolve project root"}),
    };
    let godot_exe = std::env::current_exe()
        .map(|p| p.to_string_lossy().to_string())
        .unwrap_or_default();
    if godot_exe.is_empty() {
        return json!({"error": "Cannot determine Godot executable path"});
    }

    let output = std::process::Command::new(&godot_exe)
        .args([
            "--headless",
            "--build-solutions",
            "--path",
            &project_root,
            "--quit",
        ])
        .output();

    match output {
        Ok(o) => {
            let stdout = String::from_utf8_lossy(&o.stdout).to_string();
            let stderr = String::from_utf8_lossy(&o.stderr).to_string();
            json!({
                "exit_code": o.status.code(),
                "success": o.status.success(),
                "stdout": stdout,
                "stderr": stderr,
                "project_root": project_root,
                "godot_executable": godot_exe
            })
        }
        Err(e) => json!({
            "error": format!(
                "Failed to spawn Godot: {}. Ensure Godot is built with .NET support and dotnet SDK is installed.",
                e
            ),
            "godot_executable": godot_exe,
            "project_root": project_root
        }),
    }
}
