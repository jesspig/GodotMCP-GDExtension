#include "tool_executor.hpp"

#include "logging.hpp"
#include "built_in/cmd_utils.hpp"
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

    log_tool_call(tool_name, arguments);

    Dictionary tool_result;
    try {
        tool_result = registry_.execute(tool_name, arguments);
    } catch (const std::exception &e) {
        Dictionary result;
        result["_raw_result"] = Dictionary();
        result["_exec_duration_ms"] = static_cast<double>(Time::get_singleton()->get_ticks_msec() - start_msec);
        result["_exec_error"] = format_error(kErrorInternal, String(e.what()));
        log_warn("mcp", String("tools/call FAILED (exception): ") + tool_name + String(" - ") + String(e.what()));
        return result;
    } catch (...) {
        Dictionary result;
        result["_raw_result"] = Dictionary();
        result["_exec_duration_ms"] = static_cast<double>(Time::get_singleton()->get_ticks_msec() - start_msec);
        result["_exec_error"] = format_error(kErrorInternal, "Unknown error");
        log_warn("mcp", String("tools/call FAILED (unknown exception): ") + tool_name);
        return result;
    }

    // Log result summary
    bool success = true;
    {
        String summary;
        if (tool_result.has("error")) {
            success = false;
            Variant err_val = tool_result["error"];
            if (err_val.get_type() == Variant::DICTIONARY) {
                Dictionary ed = err_val;
                summary = String("error=") + String(ed.get("code", "?")) + String(": ") + String(ed.get("message", "?"));
            } else {
                summary = String("error=") + String(err_val);
            }
        } else if (tool_result.has("success") && tool_result["success"].get_type() == Variant::BOOL && !static_cast<bool>(tool_result["success"])) {
            success = false;
            summary = String("success=false");
        } else if (tool_result.has("data") && tool_result["data"].get_type() == Variant::DICTIONARY) {
            Dictionary data = tool_result["data"];
            Array dk = data.keys();
            for (int i = 0; i < dk.size() && summary.length() < 120; i++) {
                if (!summary.is_empty()) summary += ", ";
                String k = dk[i];
                Variant v = data[k];
                summary += k + String("=");
                if (v.get_type() == Variant::STRING) {
                    String sv = v;
                    if (sv.length() > 60) sv = sv.substr(0, 60) + String("...");
                    summary += String("\"") + sv + String("\"");
                } else {
                    summary += json_stringify_safe(v);
                }
            }
        }
        if (success) {
            log_info("mcp", String("  -> ok: ") + summary);
        } else {
            log_warn("mcp", String("  -> FAIL: ") + summary);
        }
    }

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

String ToolExecutor::format_params_for_log(const Dictionary &args) {
    String param_log;
    Array param_keys = args.keys();
    for (int i = 0; i < param_keys.size(); i++) {
        if (!param_log.is_empty()) param_log += ", ";
        String k = param_keys[i];
        Variant v = args[k];
        param_log += k + String("=");
        if (v.get_type() == Variant::STRING) {
            String sv = v;
            if (sv.length() > 80) sv = sv.substr(0, 80) + String("...");
            param_log += String("\"") + sv + String("\"");
        } else {
            param_log += json_stringify_safe(v);
        }
    }
    if (param_log.length() > 200) param_log = param_log.substr(0, 200) + String("...");
    return param_log;
}

void ToolExecutor::log_tool_call(const String &tool_name, const Dictionary &args) {
    log_info("mcp", String("tools/call: ") + tool_name);
    String param_log = format_params_for_log(args);
    if (!param_log.is_empty()) {
        log_info("mcp", String("  args: ") + param_log);
    }
}

Dictionary ToolExecutor::format_success(const Dictionary &raw_result, const Dictionary &tool_result) {
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
