// commands/project_settings_ext.cpp — display/physics/rendering/layer settings

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/project_settings.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Variant ps_get(const String &k) { return ProjectSettings::get_singleton()->get_setting(k); }
void ps_set(const String &k, const Variant &v) { ProjectSettings::get_singleton()->set_setting(k, v); }
bool ps_save() { return ProjectSettings::get_singleton()->save() == OK; }
Dictionary gather(const Vector<std::pair<String, String>> &keys) {
    Dictionary r;
    for (auto &[k, a] : keys) r[a] = variant_to_json(ps_get(k));
    return r;
}
void apply(const Dictionary &args, const Vector<std::pair<String, String>> &keys) {
    for (auto &[k, a] : keys) {
        if (!args.has(a)) continue;
        Variant val = args[a];
        if (val.get_type() == Variant::STRING) {
            String s = val;
            if (s.begins_with("res://") || s.begins_with("user://")) {
                ps_set(k, s);
                continue;
            }
        }
        ps_set(k, json_to_variant(val));
    }
}
Dictionary apply_and_save(const Dictionary &args, const Vector<std::pair<String, String>> &keys) {
    apply(args, keys);
    Dictionary r; r["updated"] = true;
    if (!ps_save()) r["warning"] = "disk write failed";
    return r;
}

Vector<std::pair<String, String>> display_keys() {
    return {
        {"display/window/size/viewport_width", "viewport_width"},
        {"display/window/size/viewport_height", "viewport_height"},
        {"display/window/size/window_width_override", "window_width_override"},
        {"display/window/size/window_height_override", "window_height_override"},
        {"display/window/size/mode", "mode"},
        {"display/window/size/resizable", "resizable"},
        {"display/window/size/borderless", "borderless"},
        {"display/window/size/transparent", "transparent"},
        {"display/window/size/always_on_top", "always_on_top"},
        {"display/window/stretch/mode", "stretch_mode"},
        {"display/window/stretch/aspect", "stretch_aspect"},
        {"display/window/stretch/scale", "stretch_scale"},
        {"display/window/stretch/scale_mode", "stretch_scale_mode"},
        {"display/window/vsync/vsync_mode", "vsync_mode"},
        {"display/window/energy_saving/keep_screen_on", "keep_screen_on"},
        {"display/window/handheld/orientation", "orientation"},
    };
}
Dictionary cmd_get_display_settings(const Dictionary &) { return gather(display_keys()); }
Dictionary cmd_set_display_settings(const Dictionary &a) { return apply_and_save(a, display_keys()); }

Vector<std::pair<String, String>> project_info_keys() {
    return {
        {"application/config/name", "name"}, {"application/config/description", "description"},
        {"application/config/version", "version"}, {"application/config/icon", "icon"},
        {"application/run/main_scene", "main_scene"},
        {"application/config/use_custom_user_dir", "use_custom_user_dir"},
        {"application/config/custom_user_dir_name", "custom_user_dir_name"},
    };
}
Dictionary cmd_get_project_info(const Dictionary &) { return gather(project_info_keys()); }
Dictionary cmd_set_project_info(const Dictionary &a) { return apply_and_save(a, project_info_keys()); }

Vector<std::pair<String, String>> physics_keys() {
    return {
        {"physics/2d/default_gravity", "gravity_2d"},
        {"physics/2d/default_gravity_vector", "gravity_vector_2d"},
        {"physics/2d/default_linear_damp", "linear_damp_2d"},
        {"physics/2d/default_angular_damp", "angular_damp_2d"},
        {"physics/3d/default_gravity", "gravity_3d"},
        {"physics/3d/default_gravity_vector", "gravity_vector_3d"},
        {"physics/3d/default_linear_damp", "linear_damp_3d"},
        {"physics/3d/default_angular_damp", "angular_damp_3d"},
        {"physics/common/physics_ticks_per_second", "physics_ticks_per_second"},
        {"physics/common/max_physics_steps_per_frame", "max_physics_steps_per_frame"},
    };
}
Dictionary cmd_get_physics_settings(const Dictionary &) { return gather(physics_keys()); }
Dictionary cmd_set_physics_settings(const Dictionary &a) { return apply_and_save(a, physics_keys()); }

Vector<std::pair<String, String>> rendering_keys() {
    return {
        {"rendering/renderer/rendering_method", "rendering_method"},
        {"rendering/environment/defaults/default_clear_color", "default_clear_color"},
        {"rendering/anti_aliasing/quality/msaa_2d", "msaa_2d"},
        {"rendering/anti_aliasing/quality/msaa_3d", "msaa_3d"},
        {"rendering/anti_aliasing/quality/screen_space_aa", "screen_space_aa"},
        {"rendering/2d/snap/snap_2d_transforms_to_pixel", "snap_2d_transforms_to_pixel"},
        {"rendering/2d/snap/snap_2d_vertices_to_pixel", "snap_2d_vertices_to_pixel"},
        {"rendering/occlusion_culling/use_occlusion_culling", "use_occlusion_culling"},
    };
}
Dictionary cmd_get_rendering_settings(const Dictionary &) { return gather(rendering_keys()); }
Dictionary cmd_set_rendering_settings(const Dictionary &a) { return apply_and_save(a, rendering_keys()); }

Dictionary cmd_get_layer_names(const Dictionary &a) {
    String cat = args_string(a, "category");
    if (cat.is_empty()) return make_error("missing 'category'");
    Vector<std::pair<String, int>> cats = {{"2d_physics",32},{"2d_navigation",32},{"2d_render",20},
        {"3d_physics",32},{"3d_navigation",32},{"3d_render",20},{"avoidance",32}};
    int count = 0;
    for (auto &[c, n] : cats) { if (c == cat) { count = n; break; } }
    if (count == 0) return make_error("unknown category '" + cat + "'");
    Dictionary layers;
    for (int i = 1; i <= count; i++) {
        String key = "layer_names/" + cat + "/layer_" + String::num_int64(i);
        Variant val = ps_get(key);
        if (val.get_type() != Variant::NIL) layers[String::num_int64(i)] = variant_to_json(val);
    }
    Dictionary r; r["category"] = cat; r["layers"] = layers; return r;
}
Dictionary cmd_set_layer_names(const Dictionary &a) {
    String cat = args_string(a, "category");
    if (cat.is_empty()) return make_error("missing 'category'");
    if (!a.has("layers") || a["layers"].get_type() != Variant::DICTIONARY) return make_error("missing 'layers' object");
    Dictionary layers = a["layers"];
    Vector<std::pair<String, int>> cats = {{"2d_physics",32},{"2d_navigation",32},{"2d_render",20},
        {"3d_physics",32},{"3d_navigation",32},{"3d_render",20},{"avoidance",32}};
    int count = 0;
    for (auto &[c, n] : cats) { if (c == cat) { count = n; break; } }
    if (count == 0) return make_error("unknown category '" + cat + "'");
    int updated = 0;
    for (int i = 1; i <= count; i++) {
        String idx = String::num_int64(i);
        if (layers.has(idx)) { ps_set("layer_names/" + cat + "/layer_" + idx, json_to_variant(layers[idx])); updated++; }
    }
    Dictionary r; r["category"] = cat; r["updated"] = updated;
    if (!ps_save()) r["warning"] = "disk write failed";
    return r;
}
} // namespace

void register_project_settings_ext(HandlerRegistry &reg) {
    reg.register_tool("get_display_settings", cmd_get_display_settings);
    reg.register_tool("set_display_settings", cmd_set_display_settings);
    reg.register_tool("get_project_info", cmd_get_project_info);
    reg.register_tool("set_project_info", cmd_set_project_info);
    reg.register_tool("get_physics_settings", cmd_get_physics_settings);
    reg.register_tool("set_physics_settings", cmd_set_physics_settings);
    reg.register_tool("get_rendering_settings", cmd_get_rendering_settings);
    reg.register_tool("set_rendering_settings", cmd_set_rendering_settings);
    reg.register_tool("get_layer_names", cmd_get_layer_names);
    reg.register_tool("set_layer_names", cmd_set_layer_names);
}
} // namespace godot_mcp