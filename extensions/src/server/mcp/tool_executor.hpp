#pragma once

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

class HandlerRegistry;
struct ToolInfo;

class ToolExecutor {
public:
    ToolExecutor(HandlerRegistry &registry);
    ~ToolExecutor() = default;

    godot::Dictionary execute(const godot::String &tool_name, const godot::Dictionary &arguments);

    static godot::Dictionary extract_result(const godot::Dictionary &exec_result);

    static constexpr int kErrorInternal = -32603;
    static constexpr int kErrorInvalidRequest = -32600;

private:
    godot::String format_params_for_log(const godot::Dictionary &args);
    void log_tool_call(const godot::String &tool_name, const godot::Dictionary &args);
    godot::Dictionary format_success(const godot::Dictionary & /*raw_result*/, const godot::Dictionary &tool_result);
    godot::Dictionary format_error(int code, const godot::String &message);
    godot::Array tool_result_to_mcp_content(const godot::Dictionary &tool_result);

    HandlerRegistry &registry_;
};

} // namespace godot_mcp
