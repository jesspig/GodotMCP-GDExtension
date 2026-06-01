#pragma once

#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <vector>
#include <utility>

namespace godot_mcp {

inline Variant ps_get(const String &k) { return ProjectSettings::get_singleton()->get_setting(k); }
inline void ps_set(const String &k, const Variant &v) { ProjectSettings::get_singleton()->set_setting(k, v); }
inline bool ps_save() { return ProjectSettings::get_singleton()->save() == OK; }

using KeyAlias = std::pair<String, String>;

inline Dictionary gather(const std::vector<KeyAlias> &keys) {
    Dictionary r;
    for (auto &[k, a] : keys) r[a] = variant_to_json(ps_get(k));
    return r;
}
inline void apply(const Dictionary &args, const std::vector<KeyAlias> &keys) {
    for (auto &[k, a] : keys) {
        if (!args.has(a)) continue;
        Variant val = args[a];
        if (val.get_type() == Variant::STRING) {
            String s = val;
            if (s.begins_with("res://") || s.begins_with("user://")) { ps_set(k, s); continue; }
        }
        ps_set(k, json_to_variant(val));
    }
}
inline Dictionary apply_and_save(const Dictionary &args, const std::vector<KeyAlias> &keys) {
    apply(args, keys);
    Dictionary r; r["updated"] = true;
    if (!ps_save()) r["warning"] = "disk write failed";
    return r;
}

inline std::vector<KeyAlias> display_keys() {
    return {{"display/window/size/viewport_width","viewport_width"},{"display/window/size/viewport_height","viewport_height"},{"display/window/size/window_width_override","window_width_override"},{"display/window/size/window_height_override","window_height_override"},{"display/window/size/mode","mode"},{"display/window/size/resizable","resizable"},{"display/window/size/borderless","borderless"},{"display/window/size/transparent","transparent"},{"display/window/size/always_on_top","always_on_top"},{"display/window/stretch/mode","stretch_mode"},{"display/window/stretch/aspect","stretch_aspect"},{"display/window/stretch/scale","stretch_scale"},{"display/window/stretch/scale_mode","stretch_scale_mode"},{"display/window/vsync/vsync_mode","vsync_mode"},{"display/window/energy_saving/keep_screen_on","keep_screen_on"},{"display/window/handheld/orientation","orientation"}};
}
inline std::vector<KeyAlias> project_info_keys() {
    return {{"application/config/name","name"},{"application/config/description","description"},{"application/config/version","version"},{"application/config/icon","icon"},{"application/run/main_scene","main_scene"},{"application/config/use_custom_user_dir","use_custom_user_dir"},{"application/config/custom_user_dir_name","custom_user_dir_name"}};
}
inline std::vector<KeyAlias> physics_keys() {
    return {{"physics/2d/default_gravity","gravity_2d"},{"physics/2d/default_gravity_vector","gravity_vector_2d"},{"physics/2d/default_linear_damp","linear_damp_2d"},{"physics/2d/default_angular_damp","angular_damp_2d"},{"physics/3d/default_gravity","gravity_3d"},{"physics/3d/default_gravity_vector","gravity_vector_3d"},{"physics/3d/default_linear_damp","linear_damp_3d"},{"physics/3d/default_angular_damp","angular_damp_3d"},{"physics/common/physics_ticks_per_second","physics_ticks_per_second"},{"physics/common/max_physics_steps_per_frame","max_physics_steps_per_frame"}};
}
inline std::vector<KeyAlias> rendering_keys() {
    return {{"rendering/renderer/rendering_method","rendering_method"},{"rendering/environment/defaults/default_clear_color","default_clear_color"},{"rendering/anti_aliasing/quality/msaa_2d","msaa_2d"},{"rendering/anti_aliasing/quality/msaa_3d","msaa_3d"},{"rendering/anti_aliasing/quality/screen_space_aa","screen_space_aa"},{"rendering/2d/snap/snap_2d_transforms_to_pixel","snap_2d_transforms_to_pixel"},{"rendering/2d/snap/snap_2d_vertices_to_pixel","snap_2d_vertices_to_pixel"},{"rendering/occlusion_culling/use_occlusion_culling","use_occlusion_culling"}};
}

inline bool get_layer_count(const String &cat, int &count) {
    std::vector<std::pair<String, int>> cats = {{"2d_physics",32},{"2d_navigation",32},{"2d_render",20},{"3d_physics",32},{"3d_navigation",32},{"3d_render",20},{"avoidance",32}};
    for (auto &[c, n] : cats) { if (c == cat) { count = n; return true; } }
    return false;
}

} // namespace godot_mcp
