#pragma once

#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

inline String generate_screenshot_path(const String &viewport_type, const String &format = "png") {
    godot::Time *t = godot::Time::get_singleton();
    Dictionary dt = t->get_datetime_dict_from_system();
    String ts = String::num_int64(static_cast<int64_t>(dt["year"])) +
                String::num_int64(static_cast<int64_t>(dt["month"])).pad_zeros(2) +
                String::num_int64(static_cast<int64_t>(dt["day"])).pad_zeros(2) + String("_") +
                String::num_int64(static_cast<int64_t>(dt["hour"])).pad_zeros(2) +
                String::num_int64(static_cast<int64_t>(dt["minute"])).pad_zeros(2) +
                String::num_int64(static_cast<int64_t>(dt["second"])).pad_zeros(2);
    String ext = (format == "jpg" || format == "jpeg") ? "jpg" : "png";
    return String("res://screenshots/") + ts + String("_") + viewport_type + String(".") + ext;
}

inline String save_screenshot(const godot::Ref<godot::Image> &img, const String &path, const String &format = "png") {
    ensure_parent_dir(path);
    Error err;
    if (format == "jpg" || format == "jpeg") {
        err = img->save_jpg(path, 0.9f);
    } else {
        err = img->save_png(path);
    }
    if (err != godot::OK) {
        return String();
    }
    return path;
}

inline Dictionary capture_editor_viewport(const String &viewport_type, int viewport_index,
                                           const String &format, const String &save_path) {
    godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
    if (!ei) {
        return ToolResult::err("NO_EDITOR", "EditorInterface not available");
    }

    godot::SubViewport *viewport = nullptr;
    if (viewport_type == "3d") {
        viewport = ei->get_editor_viewport_3d(viewport_index);
    } else {
        viewport = ei->get_editor_viewport_2d();
    }
    if (!viewport) {
        return ToolResult::err("NO_VIEWPORT", "Viewport not available");
    }

    godot::Ref<godot::Image> img = viewport->get_texture()->get_image();
    if (img.is_null()) {
        return ToolResult::err("CAPTURE_FAILED", "Screenshot capture failed");
    }

    Dictionary data;
    data["width"] = img->get_width();
    data["height"] = img->get_height();
    data["format"] = format;

    String final_path = save_path.is_empty()
        ? generate_screenshot_path(viewport_type, format)
        : save_path;
    String saved = save_screenshot(img, final_path, format);
    if (!saved.is_empty()) {
        data["file_path"] = saved;
    }

    return ToolResult::ok(data);
}

inline Dictionary capture_game_viewport_impl(const String &format, const String &save_path) {
    godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
    if (!ei) {
        return ToolResult::err("NO_EDITOR", "EditorInterface not available");
    }
    if (!ei->is_playing_scene()) {
        return ToolResult::err("GAME_NOT_RUNNING", "Game not running");
    }

    godot::DisplayServer *ds = godot::DisplayServer::get_singleton();
    godot::Ref<godot::Image> img = ds->screen_get_image(0);
    if (img.is_null()) {
        return ToolResult::err("CAPTURE_FAILED", "Game screenshot failed");
    }

    Dictionary data;
    data["width"] = img->get_width();
    data["height"] = img->get_height();
    data["format"] = format;

    String final_path = save_path.is_empty()
        ? generate_screenshot_path("game", format)
        : save_path;
    String saved = save_screenshot(img, final_path, format);
    if (!saved.is_empty()) {
        data["file_path"] = saved;
    }

    return ToolResult::ok(data);
}

} // namespace godot_mcp

