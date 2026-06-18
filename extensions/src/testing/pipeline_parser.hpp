#pragma once

#include "pipeline_types.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {
namespace pipeline {

ParseResult parse_pipeline(const godot::Dictionary &config);

} // namespace pipeline
} // namespace godot_mcp
