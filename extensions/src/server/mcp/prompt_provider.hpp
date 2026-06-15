#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

class PromptProvider {
public:
    PromptProvider() = default;
    ~PromptProvider() = default;

    godot::Array list_prompts() const;
    godot::Dictionary get_prompt(const godot::String &name, const godot::Dictionary &arguments = godot::Dictionary()) const;

private:
    static godot::Dictionary make_prompt_definition(const godot::String &name, const godot::String &description, const godot::Array &arguments = godot::Array());
    static godot::Dictionary make_create_scene_prompt(const godot::Dictionary &args);
    static godot::Dictionary make_create_node_prompt(const godot::Dictionary &args);
    static godot::Dictionary make_fix_error_prompt(const godot::Dictionary &args);
    static godot::Dictionary make_explain_node_prompt(const godot::Dictionary &args);
    static godot::Dictionary make_code_review_prompt(const godot::Dictionary &args);
};

} // namespace godot_mcp
