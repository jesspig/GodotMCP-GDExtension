#pragma once

#include "../../built_in/cmd_utils.hpp"
#include "../../testing/test_engine.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

inline godot::String handle_run_tests(const godot::String &yaml_body,
                                        TestEngine &engine) {
    using namespace godot;

    const Dictionary result = engine.run(yaml_body);

    if (result.has("error")) {
        Dictionary err_resp;
        err_resp["success"] = false;
        err_resp["error"] = result["error"];
        return json_stringify_safe(err_resp);
    }

    Dictionary response;
    response["success"] = true;
    response["suite_name"] = result.get("name", "");
    response["suite_description"] = result.get("description", "");
    response["summary"] = result.get("summary", Dictionary());
    response["tests"] = result.get("tests", Array());

    return json_stringify_safe(response);
}

} // namespace godot_mcp
