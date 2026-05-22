use serde_json::{Value, json};

use godot::classes::{DirAccess, FileAccess};
use godot::prelude::GString;

use super::{CommandHandler, pipe, s};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &["find_in_file", "search_project"];

pub struct SearchCommands;

impl SearchCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for SearchCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for SearchCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn execute(&self, _args: &Value, _d: &MainThreadDispatcher) -> Result<Value, String> {
        Err("SearchCommands::execute should not be called directly".into())
    }
    fn group_name(&self) -> &str {
        "search"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl SearchCommands {
    pub async fn handle_search_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        match tool {
            "find_in_file" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_find_in_file(&a)).await)
            }
            "search_project" => {
                let a = args.clone();
                pipe(d.submit(move || cmd_search_project(&a)).await)
            }
            _ => Err(format!("Unknown search tool: {}", tool)),
        }
    }
}

struct LineMatch {
    line: usize,
    col: usize,
    text: String,
}

fn match_lines(
    source: &str,
    pattern: &str,
    literal: bool,
    case_sensitive: bool,
    max_matches: usize,
) -> Vec<LineMatch> {
    let re = if literal {
        regex::RegexBuilder::new(&regex::escape(pattern))
            .case_insensitive(!case_sensitive)
            .build()
    } else {
        regex::RegexBuilder::new(pattern)
            .case_insensitive(!case_sensitive)
            .build()
    };
    let Ok(re) = re else {
        return vec![];
    };
    let mut out = Vec::new();
    for (i, line) in source.lines().enumerate() {
        if out.len() >= max_matches {
            break;
        }
        for m in re.find_iter(line) {
            if out.len() >= max_matches {
                break;
            }
            let text = if line.len() > 240 {
                format!("{}...", &line[..240])
            } else {
                line.to_string()
            };
            out.push(LineMatch {
                line: i + 1,
                col: m.start() + 1,
                text,
            });
        }
    }
    out
}

struct FileEntry {
    path: String,
}

fn walk_dir(
    dir: &str,
    extensions: &[&str],
    include_addons: bool,
    max: usize,
    out: &mut Vec<FileEntry>,
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
            walk_dir(&full, extensions, include_addons, max, out);
        } else {
            let ext_match = if extensions.is_empty() {
                true
            } else {
                extensions.iter().any(|e| name.ends_with(e))
            };
            if ext_match && out.len() < max {
                out.push(FileEntry { path: full });
            }
        }
    }
}

fn read_file_text(path: &str) -> Result<String, String> {
    if !FileAccess::file_exists(&GString::from(path)) {
        return Err(format!("File not found: {}", path));
    }
    let content = FileAccess::get_file_as_string(&GString::from(path)).to_string();
    if content.is_empty() {
        let err = FileAccess::get_open_error();
        if err != godot::global::Error::OK {
            return Err(format!("Failed to read {}: {:?}", path, err));
        }
    }
    Ok(content)
}

fn cmd_find_in_file(args: &Value) -> Value {
    let path = s(args, "path");
    let pattern = s(args, "pattern");
    if pattern.is_empty() {
        return json!({"error": "missing 'pattern'"});
    }
    let literal = args["literal"].as_bool().unwrap_or(false);
    let case_sensitive = args["case_sensitive"].as_bool().unwrap_or(true);
    let max_matches = args["max_matches"].as_u64().unwrap_or(200) as usize;

    let source = match read_file_text(&path) {
        Ok(s) => s,
        Err(e) => return json!({"error": e}),
    };

    let matches = match_lines(&source, &pattern, literal, case_sensitive, max_matches);
    let match_json: Vec<Value> = matches
        .iter()
        .map(|m| json!({"line": m.line, "col": m.col, "text": m.text}))
        .collect();
    json!({"path": path, "matches": match_json, "count": match_json.len()})
}

fn cmd_search_project(args: &Value) -> Value {
    let pattern = s(args, "pattern");
    if pattern.is_empty() {
        return json!({"error": "missing 'pattern'"});
    }
    let literal = args["literal"].as_bool().unwrap_or(false);
    let case_sensitive = args["case_sensitive"].as_bool().unwrap_or(true);
    let extensions: Vec<String> = args["extensions"]
        .as_array()
        .map(|a| {
            a.iter()
                .filter_map(|v| v.as_str().map(|s| s.to_string()))
                .collect()
        })
        .unwrap_or_else(|| vec!["gd".into(), "cs".into(), "tscn".into(), "tres".into()]);
    let ext_refs: Vec<&str> = extensions.iter().map(|s| s.as_str()).collect();
    let root = s(args, "root");
    let root = if root.is_empty() { "res://" } else { &root };
    let include_addons = args["include_addons"].as_bool().unwrap_or(false);
    let max_matches = args["max_matches"].as_u64().unwrap_or(500) as usize;
    let max_files = args["max_files"].as_u64().unwrap_or(5000) as usize;

    let mut files = Vec::new();
    walk_dir(root, &ext_refs, include_addons, max_files, &mut files);

    let mut all_matches: Vec<Value> = Vec::new();
    let mut files_scanned: usize = 0;
    for fe in &files {
        if all_matches.len() >= max_matches {
            break;
        }
        if let Ok(source) = read_file_text(&fe.path) {
            files_scanned += 1;
            let remaining = max_matches.saturating_sub(all_matches.len());
            let hits = match_lines(&source, &pattern, literal, case_sensitive, remaining);
            for m in hits {
                all_matches.push(json!({
                    "path": fe.path,
                    "line": m.line,
                    "col": m.col,
                    "text": m.text
                }));
            }
        }
    }
    let truncated = all_matches.len() >= max_matches || files.len() >= max_files;
    json!({
        "matches": all_matches,
        "count": all_matches.len(),
        "files_scanned": files_scanned,
        "truncated": truncated
    })
}
