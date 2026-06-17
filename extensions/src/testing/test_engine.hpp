#pragma once

#include "godot_file_verifier.hpp"
#include "test_assertions.hpp"
#include "yaml_parser.hpp"

#include "../server/registry/handler_registry.hpp"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <functional>

namespace godot_mcp {

class TestEngine {
public:
    TestEngine(HandlerRegistry *registry) : registry_(registry) {}

    // Parse YAML, execute suite, return summary Dictionary:
    // {
    //   summary: { total, passed, failed, cleanup_deleted, cleanup_skipped },
    //   tests: [ { tool, description, passed, error, ... } ]
    // }
    godot::Dictionary run(const godot::String &yaml_content);

private:
    struct FileSnapshot {
        godot::PackedStringArray paths;
    };

    FileSnapshot take_snapshot();
    godot::Array track_paths(const godot::Dictionary &result);
    void cleanup(const godot::Array &tracked,
                 const FileSnapshot &before,
                 const FileSnapshot &after,
                 godot::Array &out_deleted,
                 godot::Array &out_skipped);

    // Snapshot a ProjectSettings key before set_setting/reset_setting mutates it.
    void record_setting_before_call(const godot::String &tool_name,
                                     const godot::Dictionary &args);
    // Restore every snapped setting to its pre-test value (in-memory only).
    void restore_settings();
    // Backup project.godot raw content before test execution.
    void backup_project_godot();
    // Restore project.godot raw content to pre-test state.
    void restore_project_godot();

    godot::String execute_chain(const godot::Array &chain, const char *chain_name);
    godot::Dictionary execute_test(const godot::Dictionary &test_def);

    HandlerRegistry *registry_;
    godot::Dictionary settings_snapshot_; // path → pre-test Variant
    godot::String project_godot_backup_; // raw file content snapshot
};

} // namespace godot_mcp
