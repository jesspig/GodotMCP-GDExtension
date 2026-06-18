#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {
namespace prompt_provider {

godot::Array list_prompts();
godot::Dictionary get_prompt(const godot::String &name, const godot::Dictionary &arguments = godot::Dictionary());

} // namespace prompt_provider
} // namespace godot_mcp
