
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot_mcp {

class DisconnectSignalTool : public ITool {
public:
    String name() const noexcept override { return "disconnect_signal"; }
    String category() const noexcept override { return "node_tools/signal"; }
    String brief() const noexcept override {
        return String("Disconnect a node signal");
    }
    String description() const override {
        return String("Disconnects source_node's signal_name from target_node.target_method.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Source node path");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Signal name");
            props["signal_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Target node path");
            props["target_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Method name on the target node");
            props["target_method"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "signal_name", "target_path", "target_method");
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

        Node *source = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, path, source))
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));

        Node *target = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, target_path, target))
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));

        Callable callable(target, target_method);
        if (!source->is_connected(signal_name, callable)) {
            return ToolResult::err("NOT_CONNECTED",
                String("Signal not connected: ") + signal_name);
        }

        bool is_persist = false;
        Array conns = source->get_signal_connection_list(signal_name);
        for (int64_t i = 0; i < conns.size(); i++) {
            Dictionary c = conns[i];
            Callable c_callable = c.get("callable", Callable());
            if (c_callable == callable) {
                int c_flags = c.get("flags", 0);
                if (c_flags & 4) is_persist = true;
                break;
            }
        }

        if (is_persist) {
            auto *ur = get_undo_redo();
            if (ur) {
                auto *ur_dc = begin_undo_action("MCP: Disconnect signal");
                if (ur_dc) {
                ur_dc->add_do_method(source, "disconnect", signal_name, callable);
                ur_dc->add_undo_method(source, "connect", signal_name, callable, (uint32_t)4);
                commit_undo_action(ur_dc);
                }
            } else {
                source->disconnect(signal_name, callable);
            }
        } else {
            source->disconnect(signal_name, callable);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, source);
        data["signal"] = signal_name;
        data["target"] = relative_path(ctx.root, target);
        data["target_method"] = target_method;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

