#pragma once

#include "godot_file_verifier.hpp"
#include "test_assertions.hpp"
#include "yaml_parser.hpp"

#include "../server/registry/handler_registry.hpp"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <functional>
#include <set>

namespace godot_mcp {

class TestEngine {
public:
    TestEngine(HandlerRegistry *registry) : registry_(registry) {}

    godot::Dictionary run(const godot::String &yaml_content);

private:
    struct FileSnapshot {
        godot::PackedStringArray paths;
    };

    struct CallStats {
        int call_count = 0;
        int call_success = 0;
        int call_fail = 0;
        int call_skip = 0;
        std::set<godot::String> unique_tools;
        std::set<godot::String> unique_success;
        std::set<godot::String> unique_fail;
        std::set<godot::String> unique_skip;
        godot::Array errors;
    };

    FileSnapshot take_snapshot();
    godot::Array track_paths(const godot::Dictionary &result);
    void cleanup(const godot::Array &tracked,
                 const FileSnapshot &before,
                 const FileSnapshot &after,
                 godot::Array &out_deleted,
                 godot::Array &out_skipped);

    void record_setting_before_call(const godot::String &tool_name,
                                     const godot::Dictionary &args);
    void restore_settings();
    void backup_project_godot();
    void restore_project_godot();

    godot::Dictionary execute_chain(const godot::Array &chain, const char *chain_name,
                                     const godot::String &on_failure, CallStats &stats);
    godot::Dictionary execute_test(const godot::Dictionary &test_def, CallStats &stats);

    HandlerRegistry *registry_;
    godot::Dictionary settings_snapshot_;
    godot::String project_godot_backup_;
};

} // namespace godot_mcp
