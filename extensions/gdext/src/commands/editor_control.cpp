// commands/editor_control.cpp — play/stop/refresh/info

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary cmd_play_current_scene(const Dictionary &) {
    EditorInterface::get_singleton()->play_current_scene();
    Dictionary r; r["playing"] = true; return r;
}
Dictionary cmd_play_main_scene(const Dictionary &) {
    String ms = ProjectSettings::get_singleton()->get_setting("application/run/main_scene");
    EditorInterface::get_singleton()->play_main_scene();
    Dictionary r; r["playing"] = true; r["main_scene"] = ms; return r;
}
Dictionary cmd_stop_scene(const Dictionary &) {
    EditorInterface::get_singleton()->call_deferred("stop_playing_scene");
    Dictionary r; r["stopped"] = true; return r;
}
Dictionary cmd_is_scene_playing(const Dictionary &) {
    EditorInterface *ei = EditorInterface::get_singleton();
    bool playing = ei->is_playing_scene();
    Dictionary r; r["playing"] = playing;
    if (playing) r["scene_path"] = ei->get_playing_scene(); else r["scene_path"] = Variant();
    return r;
}
Dictionary cmd_refresh_filesystem(const Dictionary &) {
    EditorFileSystem *fs = EditorInterface::get_singleton()->get_resource_filesystem();
    if (fs) { fs->scan(); Dictionary r; r["scanning"] = true; return r; }
    return make_error("EditorFileSystem not available");
}
Dictionary cmd_godot_editor_restart(const Dictionary &) {
    OS *os = OS::get_singleton();
    const String exe = os->get_executable_path();
    const String project = ProjectSettings::get_singleton()->globalize_path("res://");

    PackedStringArray args;
    args.push_back("--editor");
    args.push_back("--path");
    args.push_back(project);
    const int32_t pid = os->create_process(exe, args);
    if (pid < 0) {
        return make_error(String("Failed to launch editor (err=") + String::num_int64(pid) + String(")"));
    }

    EditorInterface::get_singleton()->get_base_control()->get_tree()->quit();
    Dictionary r; r["restarting"] = true; return r;
}
Dictionary cmd_get_editor_info(const Dictionary &) {
    EditorInterface *ei = EditorInterface::get_singleton();
    Engine *eng = Engine::get_singleton();
    Dictionary vi = eng->get_version_info();
    String ev = (String)vi.get("string", "");
    double scale = ei->get_editor_scale();
    EditorPaths *paths = ei->get_editor_paths();
    Dictionary r;
    r["engine_version"] = ev;
    r["editor_scale"] = scale;
    if (paths) {
        Dictionary pd; pd["data_dir"] = paths->get_data_dir(); pd["config_dir"] = paths->get_config_dir();
        pd["cache_dir"] = paths->get_cache_dir(); pd["project_settings_dir"] = paths->get_project_settings_dir();
        r["editor_paths"] = pd;
    }
    return r;
}
} // namespace

void register_editor_control(HandlerRegistry &reg) {
    reg.register_tool("play_current_scene", cmd_play_current_scene);
    reg.register_tool("play_main_scene", cmd_play_main_scene);
    reg.register_tool("stop_scene", cmd_stop_scene);
    reg.register_tool("is_scene_playing", cmd_is_scene_playing);
    reg.register_tool("refresh_filesystem", cmd_refresh_filesystem);
    reg.register_tool("get_editor_info", cmd_get_editor_info);
    reg.register_tool("godot_editor_restart", cmd_godot_editor_restart);
}
} // namespace godot_mcp