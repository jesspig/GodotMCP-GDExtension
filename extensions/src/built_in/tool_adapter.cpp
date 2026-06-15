#include "tool_adapter.hpp"

using namespace godot;

namespace godot_mcp {

IToolAdapter::IToolAdapter(
    const String &name, const String &category,
    const String &brief, const String &description,
    const Dictionary &input_schema, CommandFn fn,
    bool is_meta, bool needs_scene, bool needs_node,
    bool supports_undo, bool is_destructive)
    : name_(name), category_(category), brief_(brief),
      description_(description), input_schema_(input_schema),
      is_meta_(is_meta), needs_scene_(needs_scene), needs_node_(needs_node),
      supports_undo_(supports_undo),
      fn_(std::move(fn)), use_callable_(false) {
    set_is_destructive(is_destructive);
}

IToolAdapter::IToolAdapter(
    const String &name, const String &category,
    const String &brief, const String &description,
    const Dictionary &input_schema, const Callable &callable,
    bool is_meta, bool needs_scene, bool needs_node,
    bool supports_undo, bool is_destructive)
    : name_(name), category_(category), brief_(brief),
      description_(description), input_schema_(input_schema),
      is_meta_(is_meta), needs_scene_(needs_scene), needs_node_(needs_node),
      supports_undo_(supports_undo),
      callable_(callable), use_callable_(true) {
    set_is_destructive(is_destructive);
}

Dictionary IToolAdapter::execute_impl(const ToolContext &ctx) {
    if (use_callable_) {
        Variant ret = callable_.call(ctx.args);
        if (ret.get_type() == Variant::DICTIONARY) {
            return Dictionary(ret);
        }
        Dictionary err_result;
        err_result["success"] = false;
        Dictionary err_obj;
        err_obj["code"] = "INVALID_RESPONSE";
        err_obj["message"] = "Tool returned non-dictionary result";
        err_result["error"] = err_obj;
        return err_result;
    }

    if (fn_) {
        return fn_(ctx.args);
    }

    return ToolResult::err("NO_HANDLER", "No handler function configured for this tool");
}

} // namespace godot_mcp
