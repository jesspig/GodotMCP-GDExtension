use serde_json::{Value, json};

use godot::classes::file_access::ModeFlags;
use godot::classes::{DirAccess, EditorInterface, Engine, FileAccess, ProjectSettings};
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

fn get_sdk_version() -> String {
    let vi = Engine::singleton().get_version_info();
    let major: i64 = vi.get("major").map(|v| v.to()).unwrap_or(4);
    let minor: i64 = vi.get("minor").map(|v| v.to()).unwrap_or(0);
    let patch: i64 = vi.get("patch").map(|v| v.to()).unwrap_or(0);
    let status: String = vi
        .get("status")
        .map(|v| v.to::<GString>().to_string())
        .unwrap_or_else(|| "stable".to_string());

    let mut ver = format!("{}.{}.{}", major, minor, patch);
    if status != "stable" && !status.is_empty() {
        let suffix = parse_status_to_semver(&status);
        ver.push_str(&format!("-{}", suffix));
    }
    ver
}

/// Convert Godot version status to SemVer 2.0 pre-release suffix.
/// "beta2" → "beta.2", "rc1" → "rc.1", "dev" → "dev", "alpha3" → "alpha.3"
fn parse_status_to_semver(status: &str) -> String {
    let bytes = status.as_bytes();
    let split_pos = bytes.iter().position(|b| b.is_ascii_digit());
    match split_pos {
        Some(pos) if pos > 0 => {
            let (alpha, digits) = status.split_at(pos);
            format!("{}.{}", alpha, digits)
        }
        _ => status.to_string(),
    }
}

fn get_project_name() -> String {
    let ps = ProjectSettings::singleton();
    let val = ps.get_setting("dotnet/project/assembly_name");
    let custom: String = val
        .try_to::<GString>()
        .map(|g| g.to_string())
        .unwrap_or_default();
    if !custom.is_empty() {
        return custom;
    }
    let app_val = ps.get_setting("application/config/name");
    let app_name: String = app_val
        .try_to::<GString>()
        .map(|g| g.to_string())
        .unwrap_or_default();
    if !app_name.is_empty() {
        return app_name;
    }
    let project_root = ps.globalize_path(&GString::from("res://")).to_string();
    std::path::Path::new(&project_root)
        .file_name()
        .map(|n| n.to_string_lossy().to_string())
        .unwrap_or_else(|| "GodotProject".to_string())
}

fn generate_uuid() -> String {
    use std::time::{SystemTime, UNIX_EPOCH};
    let t = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default();
    let seed = t.as_nanos() as u64;
    let mut state = seed;
    let mut next_u64 = || -> u64 {
        state = state
            .wrapping_mul(6364136223846793005)
            .wrapping_add(1442695040888963407);
        let xor = ((state >> 18) ^ state) >> 27;
        (state >> 59) ^ xor
    };
    format!(
        "{:08X}-{:04X}-{:04X}-{:04X}-{:012X}",
        (next_u64() & 0xFFFF_FFFF) as u32,
        (next_u64() & 0xFFFF) as u16,
        ((next_u64() & 0x0FFF) | 0x4000) as u16,
        ((next_u64() & 0x3FFF) | 0x8000) as u16,
        next_u64() & 0xFFFF_FFFF_FFFF,
    )
}

fn sanitize_identifier(name: &str) -> String {
    if name.is_empty() {
        return "_".to_string();
    }
    let mut result = String::with_capacity(name.len());
    for ch in name.chars() {
        if result.is_empty() {
            if ch.is_ascii_alphabetic() || ch == '_' {
                result.push(ch);
            } else {
                result.push('_');
                if ch.is_ascii_alphanumeric() {
                    result.push(ch);
                }
            }
        } else if ch.is_ascii_alphanumeric() || ch == '_' || ch == '.' {
            result.push(ch);
        } else {
            result.push('_');
        }
    }
    result
}

fn write_file_utf8_no_bom(path: &str, content: &str) -> bool {
    match FileAccess::open(&GString::from(path), ModeFlags::WRITE) {
        Some(mut f) => {
            f.store_string(&GString::from(content));
            true
        }
        None => false,
    }
}

fn write_file_utf8_with_bom(path: &str, content: &str) -> bool {
    match FileAccess::open(&GString::from(path), ModeFlags::WRITE) {
        Some(mut f) => {
            let bom: [u8; 3] = [0xEF, 0xBB, 0xBF];
            let bom_var = godot::prelude::PackedByteArray::from(bom.as_slice());
            f.store_buffer(&bom_var);
            let content_bytes = content.as_bytes();
            let content_var = godot::prelude::PackedByteArray::from(content_bytes);
            f.store_buffer(&content_var);
            true
        }
        None => false,
    }
}

fn generate_csproj(project_name: &str, sdk_version: &str, enable_nativeaot: bool) -> String {
    let sanitized = sanitize_identifier(project_name);
    let root_ns = if sanitized != project_name {
        format!("\n    <RootNamespace>{}</RootNamespace>", sanitized)
    } else {
        String::new()
    };

    let aot_props = if enable_nativeaot {
        "\n    <PublishAot>true</PublishAot>".to_string()
    } else {
        String::new()
    };

    let aot_items = if enable_nativeaot {
        "\n  <ItemGroup>\n    \
         <TrimmerRootAssembly Include=\"GodotSharp\" />\n    \
         <TrimmerRootAssembly Include=\"$(TargetName)\" />\n  \
         </ItemGroup>"
            .to_string()
    } else {
        String::new()
    };

    format!(
        "<Project Sdk=\"Godot.NET.Sdk/{sdk_version}\">\n  \
         <PropertyGroup>\n    \
         <TargetFramework>net8.0</TargetFramework>\n    \
         <TargetFramework Condition=\" '$(GodotTargetPlatform)' == 'android' \">net9.0</TargetFramework>\n    \
         <EnableDynamicLoading>true</EnableDynamicLoading>{root_ns}{aot_props}\n  \
         </PropertyGroup>{aot_items}\n\
         </Project>\n"
    )
}

fn generate_sln(project_name: &str, guid: &str, csproj_filename: &str) -> String {
    let csproj_relative = csproj_filename.replace('/', "\\");

    let project_decl = format!(
        "Project(\"{{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}}\") = \"{}\", \"{}\", \"{{{}}}\"\n\
         EndProject",
        project_name, csproj_relative, guid
    );

    let configs = ["Debug", "ExportDebug", "ExportRelease"];
    let mut sln_platforms = String::new();
    let mut proj_platforms = String::new();
    for (i, cfg) in configs.iter().enumerate() {
        if i > 0 {
            sln_platforms.push('\n');
            proj_platforms.push('\n');
        }
        sln_platforms.push_str(&format!("\t{cfg}|Any CPU = {cfg}|Any CPU"));
        proj_platforms.push_str(&format!(
            "\t{{{guid}}}.{cfg}|Any CPU.ActiveCfg = {cfg}|Any CPU\n\
             \t{{{guid}}}.{cfg}|Any CPU.Build.0 = {cfg}|Any CPU"
        ));
    }

    format!(
        "Microsoft Visual Studio Solution File, Format Version 12.00\n\
         # Visual Studio 2012\n\
         {project_decl}\n\
         Global\n\
         \tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n\
         {sln_platforms}\n\
         \tEndGlobalSection\n\
         \tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n\
         {proj_platforms}\n\
         \tEndGlobalSection\n\
         EndGlobal\n"
    )
}

fn cmd_csharp_create_solution(args: &Value) -> Value {
    let enable_nativeaot = args["enable_nativeaot"].as_bool().unwrap_or(false);

    let project_root = match globalize_path("res://") {
        Some(p) => p,
        None => return json!({"error": "Cannot resolve project root"}),
    };

    let project_name = get_project_name();
    let sdk_version = get_sdk_version();
    let guid = generate_uuid();
    let csproj_filename = format!("{}.csproj", project_name);
    let sln_filename = format!("{}.sln", project_name);

    let csproj_path = format!("res://{}", csproj_filename);
    let sln_path = format!("res://{}", sln_filename);

    if FileAccess::file_exists(&GString::from(&csproj_path))
        || FileAccess::file_exists(&GString::from(&sln_path))
    {
        return json!({
            "error": format!(
                "Solution files already exist: {}, {}. Delete them first if you want to regenerate.",
                csproj_filename, sln_filename
            )
        });
    }

    let csproj_content = generate_csproj(&project_name, &sdk_version, enable_nativeaot);
    let sln_content = generate_sln(&project_name, &guid, &csproj_filename);

    if !write_file_utf8_no_bom(&csproj_path, &csproj_content) {
        return json!({"error": format!("Failed to write {}", csproj_filename)});
    }
    if !write_file_utf8_with_bom(&sln_path, &sln_content) {
        return json!({"error": format!("Failed to write {}", sln_filename)});
    }

    if let Some(mut efs) = EditorInterface::singleton().get_resource_filesystem() {
        efs.update_file(&GString::from(&csproj_path));
        efs.update_file(&GString::from(&sln_path));
    }

    json!({
        "success": true,
        "project_name": project_name,
        "sdk_version": sdk_version,
        "guid": guid,
        "csproj": csproj_filename,
        "sln": sln_filename,
        "enable_nativeaot": enable_nativeaot,
        "project_root": project_root
    })
}
