#pragma once

#include "godot_file_verifier.hpp"
#include "test_assertions.hpp"
#include "yaml_parser.hpp"

#include "../server/registry/handler_registry.hpp"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/json.hpp>
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

    godot::String execute_chain(const godot::Array &chain, const char *chain_name);
    godot::Dictionary execute_test(const godot::Dictionary &test_def);

    HandlerRegistry *registry_;
};

} // namespace godot_mcp
