// commands/script_helpers.cpp — call_method, get_variable, set_variable

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/array.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary cmd_call_method(const Dictionary &a) {
    String p = args_string(a, "node_path");
    String method = args_string(a, "method");
    if (method.is_empty()) return make_error("missing 'method'");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Array call_args;
    if (a.has("args") && a["args"].get_type() == Variant::ARRAY) {
        Array raw = a["args"];
        for (int i = 0; i < raw.size(); i++) call_args.append(json_to_variant(raw[i]));
    }
    Variant result;
    bool call_ok = true;
    String error_msg;
    try {
        const int argc = call_args.size();
        if (argc == 0) result = n->call(method);
        else if (argc == 1) result = n->call(method, call_args[0]);
        else if (argc == 2) result = n->call(method, call_args[0], call_args[1]);
        else if (argc == 3) result = n->call(method, call_args[0], call_args[1], call_args[2]);
        else if (argc == 4) result = n->call(method, call_args[0], call_args[1], call_args[2], call_args[3]);
        else if (argc == 5) result = n->call(method, call_args[0], call_args[1], call_args[2], call_args[3], call_args[4]);
        else return make_error("call_method supports at most 5 arguments");
    } catch (const std::exception &e) {
        call_ok = false;
        error_msg = String("Exception in method '") + method + String("': ") + String(e.what());
    } catch (...) {
        call_ok = false;
        error_msg = String("Unknown exception in method '") + method + String("'");
    }
    if (!call_ok) return make_error(error_msg);
    bool is_nil = result.get_type() == Variant::NIL;
    Dictionary r; r["node_path"] = p; r["method"] = method; r["result"] = variant_to_json(result);
    r["type"] = (int64_t)result.get_type();
    if (is_nil) r["hint"] = "Method returned null. Use @tool script for editor-time custom methods.";
    return r;
}
Dictionary cmd_get_variable(const Dictionary &a) {
    String p = args_string(a, "node_path");
    String var_name = args_string(a, "variable");
    if (var_name.is_empty()) return make_error("missing 'variable'");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Variant val = n->get(var_name);
    Dictionary r; r["node_path"] = p; r["variable"] = var_name;
    r["value"] = variant_to_json(val); r["type"] = (int64_t)val.get_type();
    return r;
}
Dictionary cmd_set_variable(const Dictionary &a) {
    String p = args_string(a, "node_path");
    String var_name = args_string(a, "variable");
    if (var_name.is_empty()) return make_error("missing 'variable'");
    if (!a.has("value")) return make_error("missing 'value'");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Variant gv = json_to_variant(a["value"]);
    n->set(var_name, gv);
    Variant actual = n->get(var_name);
    Dictionary r; r["node_path"] = p; r["variable"] = var_name;
    r["value"] = variant_to_json(actual); r["success"] = actual.get_type() != Variant::NIL;
    return r;
}
} // namespace

void register_script_helpers(HandlerRegistry &reg) {
    reg.register_tool("call_method", cmd_call_method);
    reg.register_tool("get_variable", cmd_get_variable);
    reg.register_tool("set_variable", cmd_set_variable);
}
} // namespace godot_mcp