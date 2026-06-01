// built_in/cmd_info.cpp — godot_info tool (replaces ping + version tools)

#include "cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace godot_mcp {
namespace {

Dictionary cmd_godot_info(const Dictionary & /*args*/) {
    Dictionary result;

    // Connection
    Dictionary conn;
    conn["status"] = "ok";
    result["connection"] = conn;

    // Engine
    Dictionary engine;
    Dictionary vi = Engine::get_singleton()->get_version_info();
    engine["version"] = vi.get("string", String());
    engine["hash"] = vi.get("hash", String());
    result["engine"] = engine;

    // Plugin — counts will be filled by the registry (passed via closure below)
    // We capture nothing here; the lambda captures the registry.

    // Project
    Dictionary project;
    ProjectSettings *ps = ProjectSettings::get_singleton();
    project["name"] = ps->get_setting("application/config/name", String());
    project["path"] = ps->globalize_path("res://");
    Variant main_scene = ps->get_setting("application/run/main_scene");
    project["main_scene"] = main_scene.get_type() != Variant::NIL ? main_scene : String();

    // Count scenes in project
    int scene_count = 0;
    // We'll use a simple approach: scan res:// for .tscn files
    // This is done through the editor filesystem if available
    EditorInterface *ei = EditorInterface::get_singleton();
    if (ei && ei->get_resource_filesystem()) {
        // EditorFileSystem doesn't have a simple count API,
        // so we report the open scenes count instead
    }
    project["scene_count"] = scene_count;
    result["project"] = project;

    // Editor state
    Dictionary editor;
    if (ei) {
        String current = ei->get_edited_scene_root()
                             ? ei->get_edited_scene_root()->get_scene_file_path()
                             : String();
        editor["current_scene"] = current;
        editor["is_playing"] = ei->is_playing_scene();

        // Open scenes
        PackedStringArray open_scenes = ei->get_open_scenes();
        Array open_arr;
        for (int i = 0; i < open_scenes.size(); ++i) {
            open_arr.push_back(open_scenes[i]);
        }
        editor["open_scenes"] = open_arr;
    }
    editor["error_count"] = 0; // TODO: integrate with editor error log
    result["editor"] = editor;

    return result;
}

} // namespace

void register_info(HandlerRegistry &reg) {
    reg.register_tool("godot_info", cmd_godot_info);
}

} // namespace godot_mcp
