#pragma once

#include "built_in/tool_base.hpp"
#include "client_registry.hpp"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class GenerateClientConfigTool : public ITool {
public:
    String name() const override { return "generate_client_config"; }
    String category() const override { return "meta_tools"; }
    String brief() const override { return "Generate MCP client configuration for various AI coding assistants"; }
    String description() const override {
        return String("Generates the appropriate MCP configuration file content for the specified AI client. "
                      "Supports 11+ clients including Claude Code, Cursor, VS Code Copilot, Codex, Trae, and more. "
                      "Can optionally write the config file directly to the project directory.");
    }

    Dictionary build_input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";

        Dictionary props;

        Dictionary client_prop;
        client_prop["type"] = "string";
        client_prop["description"] = "Target AI client name";
        Array enum_vals;
        int count;
        const ClientEntry *entries = get_entries(count);
        for (int i = 0; i < count; i++) {
            enum_vals.push_back(String(entries[i].name));
        }
        client_prop["enum"] = enum_vals;
        props["client"] = client_prop;

        Dictionary write_prop;
        write_prop["type"] = "boolean";
        write_prop["description"] = "If true, write config file to project directory";
        write_prop["default"] = false;
        props["write_to_project"] = write_prop;

        schema["properties"] = props;
        Array required;
        required.push_back("client");
        schema["required"] = required;

        return schema;
    }

    bool is_meta() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String client = ctx.args.get("client", "");
        bool write_to_project = ctx.args.get("write_to_project", false);

        if (client.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "client parameter is required");
        }

        int port = 9600;
        ProjectSettings *ps = ProjectSettings::get_singleton();
        if (ps) {
            Variant port_v = ps->get_setting("godot_mcp/http_port");
            if (port_v.get_type() == Variant::INT) {
                port = static_cast<int>(static_cast<int64_t>(port_v));
            }
        }
        String url = String("http://127.0.0.1:") + String::num_int64(port) + String("/mcp");

        int count;
        const ClientEntry *entries = get_entries(count);
        bool found = false;
        String config_path;
        String config_content;
        String format;
        for (int i = 0; i < count; i++) {
            if (client == String(entries[i].name)) {
                config_path = entries[i].config_path;
                config_content = entries[i].generator(url);
                format = entries[i].format;
                found = true;
                break;
            }
        }

        if (!found) {
            return ToolResult::err("INVALID_CLIENT", String("Unknown client: ") + client);
        }

        if (write_to_project) {
            String dir = config_path.get_base_dir();
            DirAccess::make_dir_recursive_absolute(dir);

            if (format == "json") {
                merge_json_config(config_path, config_content);
            } else if (format == "toml") {
                merge_toml_config(config_path, config_content);
            } else {
                Ref<FileAccess> fa = FileAccess::open(config_path, FileAccess::WRITE);
                if (fa.is_valid()) {
                    fa->store_string(config_content);
                    fa->close();
                }
            }
        }

        Dictionary result;
        result["client"] = client;
        result["config_path"] = config_path;
        result["config_content"] = config_content;
        result["format"] = format;
        result["scope"] = "project";
        result["url"] = url;
        return ToolResult::ok(result);
    }

private:
    static void merge_json_config(const String &path, const String &new_content) {
        Ref<FileAccess> fa = FileAccess::open(path, FileAccess::READ);
        if (fa.is_null()) {
            fa = FileAccess::open(path, FileAccess::WRITE);
            if (fa.is_valid()) {
                fa->store_string(new_content);
                fa->close();
            }
            return;
        }
        String existing = fa->get_as_text();
        fa->close();

        Variant existing_parsed = JSON::parse_string(existing);
        Variant new_parsed = JSON::parse_string(new_content);
        if (existing_parsed.get_type() != Variant::DICTIONARY) {
            fa = FileAccess::open(path, FileAccess::WRITE);
            if (fa.is_valid()) {
                fa->store_string(new_content);
                fa->close();
            }
            return;
        }
        if (new_parsed.get_type() != Variant::DICTIONARY) return;

        Dictionary existing_dict = existing_parsed;
        Dictionary new_dict = new_parsed;
        deep_merge(existing_dict, new_dict);

        fa = FileAccess::open(path, FileAccess::WRITE);
        if (fa.is_valid()) {
            fa->store_string(JSON::stringify(existing_dict, "  "));
            fa->close();
        }
    }

    static void deep_merge(Dictionary &target, const Dictionary &source) {
        Array keys = source.keys();
        for (int i = 0; i < keys.size(); i++) {
            String key = keys[i];
            Variant sv = source[key];
            Variant tv = target.get(key, Variant());
            if (sv.get_type() == Variant::DICTIONARY &&
                tv.get_type() == Variant::DICTIONARY) {
                Dictionary target_sub = tv;
                deep_merge(target_sub, sv);
                target[key] = target_sub;
            } else {
                target[key] = sv;
            }
        }
    }

    static void merge_toml_config(const String &path, const String &new_content) {
        Ref<FileAccess> fa = FileAccess::open(path, FileAccess::READ);
        if (fa.is_null()) {
            fa = FileAccess::open(path, FileAccess::WRITE);
            if (fa.is_valid()) {
                fa->store_string(new_content);
                fa->close();
            }
            return;
        }
        String existing = fa->get_as_text();
        fa->close();

        if (existing.find("[mcp_servers.godot]") >= 0) {
            return;
        }

        fa = FileAccess::open(path, FileAccess::WRITE);
        if (fa.is_valid()) {
            fa->store_string(existing + "\n" + new_content);
            fa->close();
        }
    }
};

} // namespace godot_mcp
