// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot_mcp {

class DisconnectSignalTool : public ITool {
public:
    String name() const override { return "disconnect_signal"; }
    String category() const override { return "node_tools/signal"; }
    String brief() const override {
        return String::utf8("断开节点的信号连接");
    }
    String description() const override {
        return String::utf8("断开 source_node 的 signal_name 信号与 target_node.target_method 的连接。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("源节点路径");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("信号名称");
            props["signal_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标节点路径");
            props["target_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标节点上的方法名");
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

        Node *source = resolve_node(ctx.root, path);
        if (!source)
            return ToolResult::err("NODE_NOT_FOUND", String::utf8("源节点未找到: ") + path);

        Node *target = resolve_node(ctx.root, target_path);
        if (!target)
            return ToolResult::err("NODE_NOT_FOUND", String::utf8("目标节点未找到: ") + target_path);

        Callable callable(target, target_method);
        if (!source->is_connected(signal_name, callable)) {
            return ToolResult::err("NOT_CONNECTED",
                String::utf8("信号未连接: ") + signal_name);
        }

        source->disconnect(signal_name, callable);

        Dictionary data;
        data["node"] = relative_path(ctx.root, source);
        data["signal"] = signal_name;
        data["target"] = relative_path(ctx.root, target);
        data["target_method"] = target_method;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
