
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot_mcp {

class ListSignalsTool : public ITool {
public:
    String name() const noexcept override { return "list_signals"; }
    String category() const noexcept override { return "node_tools/signal"; }
    String brief() const noexcept override {
        return String("List all available signals for a node (with parameter info)");
    }
    String description() const override {
        return String("Lists all signals defined on a given GDScript or built-in node, "
                            "including parameter names, types, and default values for each signal.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Node path (empty = root node of current edited scene)");
            props["node_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make();
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        Array signals = node->get_signal_list();
        Array result;
        for (int i = 0; i < signals.size(); i++) {
            Dictionary sig = signals[i];
            Dictionary out;
            out["name"] = sig.get("name", String());

            Array args_in = sig.get("args", Array());
            Array args_out;
            for (int j = 0; j < args_in.size(); j++) {
                Dictionary a = args_in[j];
                Dictionary arg;
                arg["name"] = a.get("name", String());
                arg["type"] = a.get("type", 0);
                args_out.push_back(arg);
            }
            out["args"] = args_out;

            Array default_args = sig.get("default_args", Array());
            out["default_arg_count"] = static_cast<int64_t>(default_args.size());

            result.push_back(out);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["signals"] = result;
        data["count"] = static_cast<int64_t>(result.size());

        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

