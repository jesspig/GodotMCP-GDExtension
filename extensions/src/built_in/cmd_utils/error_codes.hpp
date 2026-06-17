#pragma once

namespace godot_mcp {
namespace error_codes {

// Parameter errors
inline constexpr const char *MISSING_REQUIRED_PARAM = "MISSING_REQUIRED_PARAM";
inline constexpr const char *INVALID_PARAM_TYPE = "INVALID_PARAM_TYPE";

// Resource errors
inline constexpr const char *NODE_NOT_FOUND = "NODE_NOT_FOUND";
inline constexpr const char *SCENE_NOT_OPEN = "SCENE_NOT_OPEN";
inline constexpr const char *RESOURCE_NOT_FOUND = "RESOURCE_NOT_FOUND";

// Permission errors
inline constexpr const char *PERMISSION_DENIED = "PERMISSION_DENIED";

// Runtime errors
inline constexpr const char *INTERNAL_ERROR = "INTERNAL_ERROR";
inline constexpr const char *NOT_IMPLEMENTED = "NOT_IMPLEMENTED";
inline constexpr const char *TIMEOUT = "TIMEOUT";

// Bridge errors
inline constexpr const char *BRIDGE_ERROR = "BRIDGE_ERROR";
inline constexpr const char *BRIDGE_DISCONNECTED = "BRIDGE_DISCONNECTED";
inline constexpr const char *BRIDGE_TIMEOUT = "BRIDGE_TIMEOUT";

} // namespace error_codes
} // namespace godot_mcp
