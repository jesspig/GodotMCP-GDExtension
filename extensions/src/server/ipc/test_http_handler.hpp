#pragma once

#include "../../testing/test_engine.hpp"
#include "../../built_in/cmd_utils.hpp"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

// Handle a POST /run-tests request.
// Returns JSON response as a String (ready to send via send_response).
inline godot::String handle_run_tests(const godot::String &yaml_body,
                                        TestEngine &engine) {
    using namespace godot;

    Dictionary dbg;
    dbg["_h_body_len"] = yaml_body.length();
    dbg["_h_body_prefix"] = yaml_body.substr(0, 120);

    // Parse and run via TestEngine
    const Dictionary result = engine.run(yaml_body);

    dbg["_h_result_keys"] = result.keys();
    // Debug: dump names
    dbg["_h_name_get"] = result.get("name", Variant());
    dbg["_h_desc_get"] = result.get("description", Variant());
    dbg["_h_name_direct"] = result.has("name") ? result["name"] : Variant();
    // Debug: check key type / value of first key
    Array rkeys = result.keys();
    if (rkeys.size() > 0) {
        Variant first_key = rkeys[0];
        dbg["_h_first_key_str"] = String(first_key);
        dbg["_h_first_key_type"] = String::num_int64((int)first_key.get_type());
        // Try getting with the first key from keys array
        Variant val_from_first = result.get(first_key, Variant());
        dbg["_h_val_from_first"] = val_from_first;
    }

    // Check for top-level parse error
    if (result.has("error")) {
        Dictionary err_resp;
        err_resp["success"] = false;
        err_resp["error"] = result["error"];
        err_resp["_debug"] = dbg;
        return json_stringify_safe(err_resp);
    }

    Dictionary response;
    response["success"] = true;
    response["suite_name"] = result.get("name", "");
    response["suite_description"] = result.get("description", "");
    response["summary"] = result.get("summary", Dictionary());
    response["tests"] = result.get("tests", Array());

    // Forward internal debug fields (prefixed with _debug_)
    Array all_keys = result.keys();
    for (int i = 0; i < all_keys.size(); ++i) {
        const String k = all_keys[i];
        if (k.begins_with("_debug_")) {
            response[k] = result[k];
        }
    }

    response["_debug"] = dbg;

    return json_stringify_safe(response);
}

} // namespace godot_mcp
