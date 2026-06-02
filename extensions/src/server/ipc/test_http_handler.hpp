#pragma once

#include "../../testing/test_engine.hpp"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

// Handle a POST /run-tests request.
// Returns JSON response as a String (ready to send via send_response).
inline godot::String handle_run_tests(const godot::String &yaml_body,
                                       TestEngine &engine) {
    using namespace godot;

    // Parse and run via TestEngine
    const Dictionary result = engine.run(yaml_body);

    // Check for top-level parse error
    if (result.has("error")) {
        Dictionary err_resp;
        err_resp["success"] = false;
        err_resp["error"] = result["error"];
        return JSON::stringify(err_resp);
    }

    Dictionary response;
    response["success"] = true;
    response["suite_name"] = result.get("name", "");
    response["suite_description"] = result.get("description", "");
    response["summary"] = result.get("summary", Dictionary());
    response["tests"] = result.get("tests", Array());

    return JSON::stringify(response);
}

} // namespace godot_mcp
