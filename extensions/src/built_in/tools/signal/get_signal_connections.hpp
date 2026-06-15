
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace godot_mcp {

class GetSignalConnectionsTool : public ITool {
public:
    String name() const override { return "get_signal_connections"; }
    String category() const override { return "node_tools/signal"; }
    String brief() const override {
        return String("List all connections for a node signal");
    }
    String description() const override {
        return String("Returns the list of established connections for a given signal (or all signals) on the specified node, "
                            "including target object, method name and flags for each connection.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Node path");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Signal name (leave empty to list connections for all signals)");
            props["signal_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", ".");
        String signal_name = args_string(ctx.args, "signal_name", "");

        Node *node = resolve_node(ctx.root, path);
        if (!node)
            return ToolResult::err("NODE_NOT_FOUND", String("Node not found: ") + path);

        Array connections;
        if (signal_name.is_empty()) {
            Array all_signals = node->get_signal_list();
            for (int i = 0; i < all_signals.size(); i++) {
                Dictionary sig = all_signals[i];
                String name = sig.get("name", String());
                Array conns = node->get_signal_connection_list(name);
                for (int j = 0; j < conns.size(); j++) {
                    Dictionary c = conns[j];
                    Dictionary out;
                    out["signal"] = name;
                    _format_connection(out, c);
                    connections.push_back(out);
                }
            }
        } else {
            Array conns = node->get_signal_connection_list(signal_name);
            for (int j = 0; j < conns.size(); j++) {
                Dictionary c = conns[j];
                Dictionary out;
                out["signal"] = signal_name;
                _format_connection(out, c);
                connections.push_back(out);
            }
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["connections"] = connections;
        data["count"] = static_cast<int64_t>(connections.size());
        return ToolResult::ok(data);
    }

private:
    void _format_connection(Dictionary &out, const Dictionary &raw) const {
        // get_signal_connection_list returns Dictionaries with keys:
        //   "signal"   - Dictionary with signal info (has "name" key)
        //   "callable" - Callable (Godot 4)
        //   "flags"    - int (ConnectFlags)
        //   "binds"    - Array of bound arguments
        Dictionary signal_info = raw.get("signal", Dictionary());
        out["signal"] = signal_info.get("name", String());

        Callable callable = raw.get("callable", Callable());
        out["method"] = callable.get_method();
        out["target_id"] = callable.get_object_id();

        out["flags"] = raw.get("flags", 0);
        out["binds"] = raw.get("binds", Array());
    }
};

} // namespace godot_mcp

