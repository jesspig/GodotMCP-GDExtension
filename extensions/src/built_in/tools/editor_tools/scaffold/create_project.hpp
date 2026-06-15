
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class CreateProjectTool : public ITool {
public:
    String name() const noexcept override { return "create_project"; }
    String category() const noexcept override { return "editor_tools/scaffold"; }
    String brief() const noexcept override { return "Create a new Godot project"; }
    String description() const override {
        return "Creates a new Godot project at the specified path. "
               "Creates project.godot config file, main scene, default environment, and icon. "
               "WARNING: This operation writes to the filesystem and cannot be undone.";
    }

    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Project path (absolute path)";
            props["project_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Project name";
            props["project_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Template type: 2d, 3d, empty";
            p["default"] = "2d";
            props["template"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Whether to include default environment";
            p["default"] = true;
            props["include_default_env"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("project_path", "project_name");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String project_path = args_string(ctx.args, "project_path");
        String project_name = args_string(ctx.args, "project_name");
        String tmpl = args_string(ctx.args, "template", "2d");
        bool include_env = args_bool(ctx.args, "include_default_env", true);

        // Normalize path separators and ensure trailing slash
        project_path = project_path.replace("\\", "/");
        if (!project_path.ends_with("/"))
            project_path += "/";

        // Create project directory
        godot::Ref<godot::DirAccess> dir = godot::DirAccess::open(project_path);
        if (dir.is_null()) {
            Error err = godot::DirAccess::make_dir_recursive_absolute(project_path);
            if (err != Error::OK)
                return ToolResult::err("MKDIR_FAILED",
                    "Failed to create project directory: " + project_path);
            dir = godot::DirAccess::open(project_path);
            if (dir.is_null())
                return ToolResult::err("NO_DIR", "Failed to open project directory: " + project_path);
        }

        Array files_created;

        // Write project.godot
        String project_config = _make_project_config(project_name);
        Error err = _write_file(project_path + "project.godot", project_config);
        if (err != Error::OK)
            return ToolResult::err("WRITE_FAILED", "Failed to create project.godot");
        files_created.append(project_path + "project.godot");

        // Create icon.svg
        String icon_content = _make_icon_svg();
        err = _write_file(project_path + "icon.svg", icon_content);
        if (err == Error::OK)
            files_created.append(project_path + "icon.svg");

        // Create main scene if not empty template
        if (tmpl != "empty") {
            String scene_content = _make_main_scene(tmpl);
            err = _write_file(project_path + "main.tscn", scene_content);
            if (err != Error::OK)
                return ToolResult::err("WRITE_FAILED", "Failed to create main.tscn");
            files_created.append(project_path + "main.tscn");

            // Add main scene to project settings config
            String config_line = "\n[application]\n\nconfig/name=\"" + project_name + "\"\n";
            config_line += "run/main_scene=\"res://main.tscn\"\n";
            err = _append_file(project_path + "project.godot", config_line);
            if (err != Error::OK)
                return ToolResult::err("WRITE_FAILED", "Failed to update project.godot");
        }

        // Create default_env.tres
        if (include_env) {
            String env_content = _make_default_env();
            err = _write_file(project_path + "default_env.tres", env_content);
            if (err == Error::OK)
                files_created.append(project_path + "default_env.tres");

            String env_config = "\n[rendering]\n\nenvironment/default_environment=\"res://default_env.tres\"\n";
            _append_file(project_path + "project.godot", env_config);
        }

        // Notify editor filesystem
        notify_file_changed(project_path + "project.godot");

        Dictionary data;
        data["project_path"] = project_path;
        data["project_name"] = project_name;
        data["template"] = tmpl;
        data["files_created"] = files_created;
        return ToolResult::ok(data);
    }

private:
    static String _make_project_config(const String &name) {
        String config;
        config += "; Engine configuration file.\n";
        config += "; It's stored by default in the project root, but it's also possible to store it elsewhere.\n";
        config += "; This file is generated by the engine. Do not modify it manually.\n\n";
        config += "[application]\n\n";
        config += "config/name=\"" + name + "\"\n";
        config += "config/description=\"\"\n";
        config += "config/tags=[]\n";
        config += "config/icon=\"res://icon.svg\"\n\n";
        config += "[rendering]\n\n";
        config += "renderer/rendering_method=\"mobile\"\n";
        config += "renderer/rendering_method.mobile=\"mobile\"\n";
        return config;
    }

    static String _make_main_scene(const String &tmpl) {
        String root_type = "Node2D";
        if (tmpl == "3d")
            root_type = "Node3D";

        String scene;
        scene += "[gd_scene load_steps=1 format=4 uid=\"uid://dummy\"]\n\n";
        scene += "[node name=\"Main\" type=\"" + root_type + "\"]\n";
        return scene;
    }

    static String _make_default_env() {
        String env;
        env += "[gd_resource type=\"Environment\" load_steps=2 format=4]\n\n";
        env += "[sub_resource type=\"Sky\" id=1]\n";
        env += "sky_material = SubResource(2)\n\n";
        env += "[sub_resource type=\"ProceduralSkyMaterial\" id=2]\n";
        env += "sky_top_color = Color(0.047, 0.439, 0.906)\n";
        env += "sky_horizon_color = Color(0.627, 0.808, 0.961)\n";
        env += "ground_bottom_color = Color(0.282, 0.286, 0.282)\n";
        env += "ground_horizon_color = Color(0.608, 0.616, 0.62)\n\n";
        env += "[resource]\n";
        env += "background_mode = 2\n";
        env += "background_sky = SubResource(1)\n";
        return env;
    }

    static String _make_icon_svg() {
        String svg;
        svg += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 64 64\" width=\"64\" height=\"64\">\n";
        svg += "  <rect width=\"64\" height=\"64\" rx=\"8\" fill=\"#478cbf\"/>\n";
        svg += "  <text x=\"32\" y=\"42\" font-size=\"28\" fill=\"white\" text-anchor=\"middle\" font-family=\"sans-serif\">G</text>\n";
        svg += "</svg>\n";
        return svg;
    }

    static Error _write_file(const String &path, const String &content) {
        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::WRITE);
        if (file.is_null())
            return Error::FAILED;
        file->store_string(content);
        file->close();
        return Error::OK;
    }

    static Error _append_file(const String &path, const String &content) {
        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::READ_WRITE);
        if (file.is_null())
            return Error::FAILED;
        file->seek_end();
        file->store_string(content);
        file->close();
        return Error::OK;
    }
};

} // namespace godot_mcp
