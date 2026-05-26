// =====================================================================
// protocol/ipc_types.hpp — Wire-format helpers for the MCP IPC protocol.
//
// The protocol carries JSON Dictionaries between the Python MCP server
// and the GDExtension. This header centralises the format constants and
// the few small construction helpers used by the WebSocket layer.
//
// Wire schema:
//   Request:       {"id": <int>, "method": "tool_call",
//                   "params": {"tool": <str>, "args": <obj>}}
//   Response OK:   {"id": <int>, "result": {"status": "success",
//                                            "data": <obj>}}
//   Response Err:  {"id": <int>, "result": {"status": "error",
//                                            "code": <str>, "message": <str>}}
//   Notification:  {"type": "notification", "event": <str>, "data": <obj>}
// =====================================================================

#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

// ----- Status strings used in the "status" field of a response. ----------
constexpr const char *kStatusSuccess = "success";
constexpr const char *kStatusError = "error";

// ----- Error codes (mirrors crates/core/src/protocol.rs). -----------------
constexpr const char *kErrCodeInvalidRequest = "INVALID_REQUEST";
constexpr const char *kErrCodeUnknownTool = "UNKNOWN_TOOL";
constexpr const char *kErrCodeToolFailed = "TOOL_FAILED";
constexpr const char *kErrCodeInternal = "INTERNAL_ERROR";

// Construct a successful response payload (the value under "result").
inline godot::Dictionary make_success_result(const godot::Variant &data) {
    godot::Dictionary d;
    d["status"] = kStatusSuccess;
    d["data"] = data;
    return d;
}

// Construct an error response payload (the value under "result").
inline godot::Dictionary make_error_result(const godot::String &code,
                                           const godot::String &message) {
    godot::Dictionary d;
    d["status"] = kStatusError;
    d["code"] = code;
    d["message"] = message;
    return d;
}

// Wrap a "result" dict into a full response envelope with the request id.
inline godot::Dictionary make_response(const godot::Variant &id,
                                       const godot::Dictionary &result) {
    godot::Dictionary d;
    d["id"] = id;
    d["result"] = result;
    return d;
}

// Build a notification envelope.
inline godot::Dictionary make_notification(const godot::String &event,
                                           const godot::Variant &data) {
    godot::Dictionary d;
    d["type"] = "notification";
    d["event"] = event;
    d["data"] = data;
    return d;
}

}  // namespace godot_mcp
