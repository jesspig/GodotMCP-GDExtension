#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace godot_mcp {

class ConnectSignalTool : public ITool {
public:
    String name() const override { return "connect_signal"; }
    String category() const override { return "node_tools/signal"; }
    String brief() const override {
        return String("Connect a node signal to a target method");
    }
    String description() const override {
        return String("Connects a signal from source_node to target_method on target_node. "
                             "Supports flags like CONNECT_DEFERRED(1), CONNECT_ONESHOT(2), etc.");
    }
    String category_description() const override {
        return "Signal connection tools for connecting, disconnecting, and inspecting node signals";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Source node path (the node emitting the signal)");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Signal name to connect");
            props["signal_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Target node path (the node receiving the signal)");
            props["target_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Method name on the target node");
            props["target_method"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String("Connection flags (1=DEFERRED, 2=ONESHOT, 4=PERSIST, 8=REFERENCE_COUNTED)");
            props["flags"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        Array required = Array::make("node_path", "signal_name", "target_path", "target_method");
        s["required"] = required;
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path");
        String signal_name = args_string(ctx.args, "signal_name");
        String target_path = args_string(ctx.args, "target_path");
        String target_method = args_string(ctx.args, "target_method");
        int flags = (int)args_int(ctx.args, "flags", 0);

        if (signal_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("signal_name cannot be empty"));
        }
        if (target_method.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("target_method cannot be empty"));
        }

        Node *source = resolve_node(ctx.root, path);
        if (!source) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("Source node not found: ") + path);
        }

        Node *target = resolve_node(ctx.root, target_path);
        if (!target) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("Target node not found: ") + target_path);
        }

        Callable callable(target, target_method);
        bool is_persist = (flags & 4) != 0;
        if (is_persist) {
            godot::EditorUndoRedoManager *ur = get_undo_redo();
            if (ur) {
                ur->create_action("MCP: Connect signal", godot::UndoRedo::MERGE_DISABLE, ctx.root);
                ur->add_do_method(source, "connect", signal_name, callable, (uint32_t)flags);
                ur->add_undo_method(source, "disconnect", signal_name, callable);
                ur->commit_action();
            } else {
                Error err = source->connect(signal_name, callable, (uint32_t)flags);
                if (err != godot::OK) {
                    String msg = String("Signal connect failed, error: ") + godot::itos((int)err);
                    return ToolResult::err("CONNECT_FAILED", msg);
                }
            }
        } else {
            Error err = source->connect(signal_name, callable, (uint32_t)flags);
            if (err != godot::OK) {
                String msg = String("Signal connect failed, error: ") + godot::itos((int)err);
                return ToolResult::err("CONNECT_FAILED", msg);
            }
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, source);
        data["signal"] = signal_name;
        data["target"] = relative_path(ctx.root, target);
        data["target_method"] = target_method;
        if (flags > 0) data["flags"] = flags;

        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
