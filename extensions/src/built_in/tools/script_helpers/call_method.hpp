// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/variant/array.hpp>
#include <stdexcept>

namespace godot_mcp {

class CallMethodTool : public ITool {
public:
    String name() const override { return "call_method"; }
    String category() const override { return "script/helpers"; }
    String brief() const override { return "Call a method on a node"; }
    String category_description() const override { return String::utf8("脚本辅助功能（调用方法等）"); }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Calls a named method on a node with optional arguments. Supports up to 5 args."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["method"] = [](){Dictionary d;d["type"]="string";d["description"]="Method name";return d;}();
        p["args"] = [](){Dictionary d;d["type"]="array";d["description"]="Method arguments (JSON array)";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); r.push_back("method"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String method = args_string(ctx.args, "method");
        if (method.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'method'");
        Array call_args;
        if (ctx.args.has("args") && ctx.args["args"].get_type() == Variant::ARRAY) {
            Array raw = ctx.args["args"];
            for (int i = 0; i < raw.size(); i++) call_args.append(json_to_variant(raw[i]));
        }
        Variant result; bool call_ok = true; String error_msg;
        try {
            const int argc = call_args.size();
            if (argc == 0) result = ctx.node->call(method);
            else if (argc == 1) result = ctx.node->call(method, call_args[0]);
            else if (argc == 2) result = ctx.node->call(method, call_args[0], call_args[1]);
            else if (argc == 3) result = ctx.node->call(method, call_args[0], call_args[1], call_args[2]);
            else if (argc == 4) result = ctx.node->call(method, call_args[0], call_args[1], call_args[2], call_args[3]);
            else if (argc == 5) result = ctx.node->call(method, call_args[0], call_args[1], call_args[2], call_args[3], call_args[4]);
            else return ToolResult::err("TOO_MANY_ARGS", "call_method supports at most 5 arguments");
        } catch (const std::exception &e) { call_ok = false; error_msg = String("Exception: ") + String(e.what());
        } catch (...) { call_ok = false; error_msg = "Unknown exception"; }
        if (!call_ok) return ToolResult::err("CALL_FAILED", error_msg);
        bool is_nil = result.get_type() == Variant::NIL;
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["method"] = method;
        d["result"] = variant_to_json(result); d["type"] = (int64_t)result.get_type();
        if (is_nil) d["hint"] = "Method returned null. Use @tool script for editor-time custom methods.";
        return ToolResult::ok(d);
    }
};

} // namespace
