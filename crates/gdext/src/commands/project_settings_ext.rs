use serde_json::{Value, json};

use godot::classes::ProjectSettings;
use godot::obj::Singleton;
use godot::prelude::GString;

use super::{CommandHandler, j2v, pipe, s, v2j};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "get_display_settings",
    "set_display_settings",
    "get_project_info",
    "set_project_info",
    "get_physics_settings",
    "set_physics_settings",
    "get_rendering_settings",
    "set_rendering_settings",
    "get_layer_names",
    "set_layer_names",
];

pub struct ProjectSettingsExtCommands;

impl ProjectSettingsExtCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for ProjectSettingsExtCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for ProjectSettingsExtCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn execute(&self, _args: &Value, _d: &MainThreadDispatcher) -> Result<Value, String> {
        Err("ProjectSettingsExtCommands::execute should not be called directly".into())
    }
    fn group_name(&self) -> &str {
        "project_settings_ext"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl ProjectSettingsExtCommands {
    pub async fn handle_project_settings_ext_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "get_display_settings" => pipe(d.submit(cmd_get_display_settings).await),
            "set_display_settings" => pipe(d.submit(move || cmd_set_display_settings(&a)).await),
            "get_project_info" => pipe(d.submit(cmd_get_project_info).await),
            "set_project_info" => pipe(d.submit(move || cmd_set_project_info(&a)).await),
            "get_physics_settings" => pipe(d.submit(cmd_get_physics_settings).await),
            "set_physics_settings" => pipe(d.submit(move || cmd_set_physics_settings(&a)).await),
            "get_rendering_settings" => pipe(d.submit(cmd_get_rendering_settings).await),
            "set_rendering_settings" => {
                pipe(d.submit(move || cmd_set_rendering_settings(&a)).await)
            }
            "get_layer_names" => pipe(d.submit(move || cmd_get_layer_names(&a)).await),
            "set_layer_names" => pipe(d.submit(move || cmd_set_layer_names(&a)).await),
            _ => Err(format!("Unknown project_settings_ext tool: {}", tool)),
        }
    }
}

fn ps_get(key: &str) -> Value {
    let ps = ProjectSettings::singleton();
    v2j(&ps.get_setting(&GString::from(key)))
}

fn ps_set(key: &str, val: &Value) {
    let mut ps = ProjectSettings::singleton();
    ps.set_setting(&GString::from(key), &j2v(val));
}

fn ps_save() -> bool {
    ProjectSettings::singleton().save() == godot::global::Error::OK
}

fn apply_and_save(result: &mut Value) {
    if !ps_save() {
        result.as_object_mut().unwrap().insert(
            "warning".into(),
            json!("saved to memory but disk write failed"),
        );
    }
}

fn gather(keys: &[(&str, &str)]) -> Value {
    let mut map = serde_json::Map::new();
    for (key, alias) in keys {
        map.insert(alias.to_string(), ps_get(key));
    }
    Value::Object(map)
}

fn apply(args: &Value, keys: &[(&str, &str)]) {
    for (key, alias) in keys {
        if let Some(v) = args.get(alias) {
            ps_set(key, v);
        }
    }
}

// ── Display ──────────────────────────────────────────────────────────

fn cmd_get_display_settings() -> Value {
    gather(&[
        ("display/window/size/viewport_width", "viewport_width"),
        ("display/window/size/viewport_height", "viewport_height"),
        (
            "display/window/size/window_width_override",
            "window_width_override",
        ),
        (
            "display/window/size/window_height_override",
            "window_height_override",
        ),
        ("display/window/size/mode", "mode"),
        ("display/window/size/resizable", "resizable"),
        ("display/window/size/borderless", "borderless"),
        ("display/window/size/transparent", "transparent"),
        ("display/window/size/always_on_top", "always_on_top"),
        ("display/window/stretch/mode", "stretch_mode"),
        ("display/window/stretch/aspect", "stretch_aspect"),
        ("display/window/stretch/scale", "stretch_scale"),
        ("display/window/stretch/scale_mode", "stretch_scale_mode"),
        ("display/window/vsync/vsync_mode", "vsync_mode"),
        (
            "display/window/energy_saving/keep_screen_on",
            "keep_screen_on",
        ),
        ("display/window/handheld/orientation", "orientation"),
    ])
}

fn cmd_set_display_settings(args: &Value) -> Value {
    apply(
        args,
        &[
            ("display/window/size/viewport_width", "viewport_width"),
            ("display/window/size/viewport_height", "viewport_height"),
            (
                "display/window/size/window_width_override",
                "window_width_override",
            ),
            (
                "display/window/size/window_height_override",
                "window_height_override",
            ),
            ("display/window/size/mode", "mode"),
            ("display/window/size/resizable", "resizable"),
            ("display/window/size/borderless", "borderless"),
            ("display/window/size/transparent", "transparent"),
            ("display/window/size/always_on_top", "always_on_top"),
            ("display/window/stretch/mode", "stretch_mode"),
            ("display/window/stretch/aspect", "stretch_aspect"),
            ("display/window/stretch/scale", "stretch_scale"),
            ("display/window/stretch/scale_mode", "stretch_scale_mode"),
            ("display/window/vsync/vsync_mode", "vsync_mode"),
            (
                "display/window/energy_saving/keep_screen_on",
                "keep_screen_on",
            ),
            ("display/window/handheld/orientation", "orientation"),
        ],
    );
    let mut result = json!({"updated": true});
    apply_and_save(&mut result);
    result
}

// ── Project info ─────────────────────────────────────────────────────

fn cmd_get_project_info() -> Value {
    gather(&[
        ("application/config/name", "name"),
        ("application/config/description", "description"),
        ("application/config/version", "version"),
        ("application/config/icon", "icon"),
        ("application/run/main_scene", "main_scene"),
        (
            "application/config/use_custom_user_dir",
            "use_custom_user_dir",
        ),
        (
            "application/config/custom_user_dir_name",
            "custom_user_dir_name",
        ),
    ])
}

fn cmd_set_project_info(args: &Value) -> Value {
    apply(
        args,
        &[
            ("application/config/name", "name"),
            ("application/config/description", "description"),
            ("application/config/version", "version"),
            ("application/config/icon", "icon"),
            ("application/run/main_scene", "main_scene"),
            (
                "application/config/use_custom_user_dir",
                "use_custom_user_dir",
            ),
            (
                "application/config/custom_user_dir_name",
                "custom_user_dir_name",
            ),
        ],
    );
    let mut result = json!({"updated": true});
    apply_and_save(&mut result);
    result
}

// ── Physics ──────────────────────────────────────────────────────────

fn cmd_get_physics_settings() -> Value {
    gather(&[
        ("physics/2d/default_gravity", "gravity_2d"),
        ("physics/2d/default_gravity_vector", "gravity_vector_2d"),
        ("physics/2d/default_linear_damp", "linear_damp_2d"),
        ("physics/2d/default_angular_damp", "angular_damp_2d"),
        ("physics/3d/default_gravity", "gravity_3d"),
        ("physics/3d/default_gravity_vector", "gravity_vector_3d"),
        ("physics/3d/default_linear_damp", "linear_damp_3d"),
        ("physics/3d/default_angular_damp", "angular_damp_3d"),
        (
            "physics/common/physics_ticks_per_second",
            "physics_ticks_per_second",
        ),
        (
            "physics/common/max_physics_steps_per_frame",
            "max_physics_steps_per_frame",
        ),
    ])
}

fn cmd_set_physics_settings(args: &Value) -> Value {
    apply(
        args,
        &[
            ("physics/2d/default_gravity", "gravity_2d"),
            ("physics/2d/default_gravity_vector", "gravity_vector_2d"),
            ("physics/2d/default_linear_damp", "linear_damp_2d"),
            ("physics/2d/default_angular_damp", "angular_damp_2d"),
            ("physics/3d/default_gravity", "gravity_3d"),
            ("physics/3d/default_gravity_vector", "gravity_vector_3d"),
            ("physics/3d/default_linear_damp", "linear_damp_3d"),
            ("physics/3d/default_angular_damp", "angular_damp_3d"),
            (
                "physics/common/physics_ticks_per_second",
                "physics_ticks_per_second",
            ),
            (
                "physics/common/max_physics_steps_per_frame",
                "max_physics_steps_per_frame",
            ),
        ],
    );
    let mut result = json!({"updated": true});
    apply_and_save(&mut result);
    result
}

// ── Rendering ────────────────────────────────────────────────────────

fn cmd_get_rendering_settings() -> Value {
    gather(&[
        ("rendering/renderer/rendering_method", "rendering_method"),
        (
            "rendering/environment/defaults/default_clear_color",
            "default_clear_color",
        ),
        ("rendering/anti_aliasing/quality/msaa_2d", "msaa_2d"),
        ("rendering/anti_aliasing/quality/msaa_3d", "msaa_3d"),
        (
            "rendering/anti_aliasing/quality/screen_space_aa",
            "screen_space_aa",
        ),
        (
            "rendering/2d/snap/snap_2d_transforms_to_pixel",
            "snap_2d_transforms_to_pixel",
        ),
        (
            "rendering/2d/snap/snap_2d_vertices_to_pixel",
            "snap_2d_vertices_to_pixel",
        ),
        (
            "rendering/occlusion_culling/use_occlusion_culling",
            "use_occlusion_culling",
        ),
    ])
}

fn cmd_set_rendering_settings(args: &Value) -> Value {
    apply(
        args,
        &[
            ("rendering/renderer/rendering_method", "rendering_method"),
            (
                "rendering/environment/defaults/default_clear_color",
                "default_clear_color",
            ),
            ("rendering/anti_aliasing/quality/msaa_2d", "msaa_2d"),
            ("rendering/anti_aliasing/quality/msaa_3d", "msaa_3d"),
            (
                "rendering/anti_aliasing/quality/screen_space_aa",
                "screen_space_aa",
            ),
            (
                "rendering/2d/snap/snap_2d_transforms_to_pixel",
                "snap_2d_transforms_to_pixel",
            ),
            (
                "rendering/2d/snap/snap_2d_vertices_to_pixel",
                "snap_2d_vertices_to_pixel",
            ),
            (
                "rendering/occlusion_culling/use_occlusion_culling",
                "use_occlusion_culling",
            ),
        ],
    );
    let mut result = json!({"updated": true});
    apply_and_save(&mut result);
    result
}

// ── Layer names ──────────────────────────────────────────────────────

static LAYER_CATEGORIES: &[(&str, usize)] = &[
    ("2d_physics", 32),
    ("2d_navigation", 32),
    ("2d_render", 20),
    ("3d_physics", 32),
    ("3d_navigation", 32),
    ("3d_render", 20),
    ("avoidance", 32),
];

fn cmd_get_layer_names(args: &Value) -> Value {
    let category = s(args, "category");
    if category.is_empty() {
        return json!({"error": "missing 'category'"});
    }
    let count = LAYER_CATEGORIES
        .iter()
        .find(|(c, _)| *c == category)
        .map(|(_, n)| *n);
    let Some(count) = count else {
        return json!({"error": format!("unknown category '{}'. Valid: 2d_physics, 2d_navigation, 2d_render, 3d_physics, 3d_navigation, 3d_render, avoidance", category)});
    };
    let mut layers = serde_json::Map::new();
    for i in 1..=count {
        let key = format!("layer_names/{}/layer_{}", category, i);
        let val = ps_get(&key);
        if !val.is_null() {
            layers.insert(format!("{}", i), val);
        }
    }
    json!({"category": category, "layers": layers})
}

fn cmd_set_layer_names(args: &Value) -> Value {
    let category = s(args, "category");
    if category.is_empty() {
        return json!({"error": "missing 'category'"});
    }
    let count = LAYER_CATEGORIES
        .iter()
        .find(|(c, _)| *c == category)
        .map(|(_, n)| *n);
    let Some(count) = count else {
        return json!({"error": format!("unknown category '{}'. Valid: 2d_physics, 2d_navigation, 2d_render, 3d_physics, 3d_navigation, 3d_render, avoidance", category)});
    };
    let layers = match args.get("layers") {
        Some(Value::Object(m)) => m,
        _ => return json!({"error": "missing or invalid 'layers' (expected object)"}),
    };
    let mut updated = 0usize;
    for i in 1..=count {
        if let Some(val) = layers
            .get(&i.to_string())
            .or_else(|| layers.get(&format!("{}", i)))
        {
            let key = format!("layer_names/{}/layer_{}", category, i);
            ps_set(&key, val);
            updated += 1;
        }
    }
    let mut result = json!({"category": category, "updated": updated});
    apply_and_save(&mut result);
    result
}
