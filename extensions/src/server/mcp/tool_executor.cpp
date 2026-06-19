#include "tool_executor.hpp"

#include "logging.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <exception>

using namespace godot;

namespace godot_mcp {

ToolExecutor::ToolExecutor(HandlerRegistry &registry)
    : registry_(registry) {}

Dictionary ToolExecutor::execute(const String &tool_name, const Dictionary &arguments) {
    const uint64_t start_msec = Time::get_singleton()->get_ticks_msec();

    String perm_policy = OS::get_singleton()->get_environment("GODOT_MCP_PERMISSION");
    if (perm_policy.is_empty()) perm_policy = "allow_all";

    const ToolInfo *info = registry_.find_tool_info(tool_name);

    if (info && info->is_destructive && perm_policy == "deny_destructive") {
        Dictionary result;
        result["_raw_result"] = Dictionary();
        result["_exec_duration_ms"] = static_cast<double>(Time::get_singleton()->get_ticks_msec() - start_msec);
        result["_exec_error"] = format_error(kErrorInvalidRequest, "Destructive tool denied by permission policy: " + tool_name);
        return result;
    }

    // Confirmation check for destructive tools under confirm_destructive policy
    if (info && info->is_destructive && perm_policy == "confirm_destructive") {
        bool confirmed = arguments.get("_confirm", false);
        if (!confirmed) {
            Dictionary err_data;
            err_data["destructive"] = true;
            err_data["tool_name"] = tool_name;
            err_data["message"] = String("This tool is destructive and requires explicit confirmation. "
                                          "Set '_confirm' to true to proceed. Tool: ") + tool_name;

            Dictionary result;
            result["_raw_result"] = Dictionary();
            result["_exec_duration_ms"] = static_cast<double>(Time::get_singleton()->get_ticks_msec() - start_msec);
            Dictionary err_info = format_error(kErrorInvalidRequest,
                String("Confirmation required for destructive tool: ") + tool_name);
            err_info["data"] = err_data;
            result["_exec_error"] = err_info;
            return result;
        }
    }

    Dictionary tool_result;
    try {
        tool_result = registry_.execute(tool_name, arguments);
    } catch (const std::exception &e) {
        Dictionary result;
        result["_raw_result"] = Dictionary();
        result["_exec_duration_ms"] = static_cast<double>(Time::get_singleton()->get_ticks_msec() - start_msec);
        result["_exec_error"] = format_error(kErrorInternal, String(e.what()));
        log_warn("executor", String("tools/call FAILED (exception): ") + tool_name + String(" - ") + String(e.what()));
        return result;
    } catch (...) {
        Dictionary result;
        result["_raw_result"] = Dictionary();
        result["_exec_duration_ms"] = static_cast<double>(Time::get_singleton()->get_ticks_msec() - start_msec);
        result["_exec_error"] = format_error(kErrorInternal, "Unknown error");
        log_warn("executor", String("tools/call FAILED (unknown exception): ") + tool_name);
        return result;
    }

    bool success = !tool_result.has("error") && (!tool_result.has("success") || static_cast<bool>(tool_result.get("success", true)));

    const double duration_ms = static_cast<double>(Time::get_singleton()->get_ticks_msec() - start_msec);

    // Build error response if tool_result signals failure
    if (!success || tool_result.has("error")) {
        Dictionary err_data;
        int err_code = kErrorInternal;
        String err_msg = "Tool execution failed";

        if (tool_result.has("error")) {
            Variant err_val = tool_result["error"];
            if (err_val.get_type() == Variant::DICTIONARY) {
                err_msg = Dictionary(err_val).get("message", "Unknown error");
            } else {
                err_msg = err_val;
            }
        }

        if (tool_result.has("recoverable") && tool_result["recoverable"].get_type() == Variant::BOOL && static_cast<bool>(tool_result["recoverable"])) {
            err_data["recoverable"] = true;
            if (tool_result.has("suggestion")) {
                err_data["suggestion"] = tool_result["suggestion"];
            }
        }

        Dictionary result;
        result["_raw_result"] = tool_result;
        result["_exec_duration_ms"] = duration_ms;
        Dictionary err_dict;
        err_dict["code"] = err_code;
        err_dict["message"] = err_msg;
        if (!err_data.is_empty()) {
            err_dict["data"] = err_data;
        }
        result["_exec_error"] = err_dict;
        return result;
    }

    // Format success response
    Dictionary mcp_result = format_success(tool_result, tool_result);
    mcp_result["_raw_result"] = tool_result;
    mcp_result["_exec_duration_ms"] = duration_ms;
    return mcp_result;
}

Dictionary ToolExecutor::extract_result(const Dictionary &exec_result) {
    if (exec_result.has("_raw_result")) {
        return exec_result["_raw_result"];
    }
    return Dictionary();
}

Dictionary ToolExecutor::format_success(const Dictionary & /*raw_result*/, const Dictionary &tool_result) {
    Dictionary result;
    result["content"] = tool_result_to_mcp_content(tool_result);
    result["isError"] = false;

    if (tool_result.has("meta")) {
        result["meta"] = tool_result["meta"];
    }
    if (tool_result.has("confirm")) {
        result["confirm"] = tool_result["confirm"];
    }

    // Merge extra fields from tool_result (excluding error, success, data)
    const Array dict_keys = tool_result.keys();
    for (int i = 0; i < dict_keys.size(); ++i) {
        const String k = dict_keys[i];
        if (k != "error" && k != "success" && k != "data") {
            result[k] = tool_result[k];
        }
    }

    // Merge fields from data sub-dictionary
    if (tool_result.has("data") && tool_result["data"].get_type() == Variant::DICTIONARY) {
        const Dictionary data = tool_result["data"];
        const Array data_keys = data.keys();
        for (int i = 0; i < data_keys.size(); ++i) {
            const String k = data_keys[i];
            if (!result.has(k)) {
                result[k] = data[k];
            }
        }
    }

    return result;
}

Dictionary ToolExecutor::format_error(int code, const String &message) {
    Dictionary err;
    err["code"] = code;
    err["message"] = message;
    return err;
}

Array ToolExecutor::tool_result_to_mcp_content(const Dictionary &tool_result) {
    Array content;
    Dictionary text_item;
    text_item["type"] = "text";
    text_item["text"] = JSON::stringify(tool_result);
    content.push_back(text_item);
    return content;
}

} // namespace godot_mcp
