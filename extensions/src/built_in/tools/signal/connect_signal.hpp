// @tool register
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
        return String::utf8("将节点的信号连接到目标节点的方法");
    }
    String description() const override {
        return String::utf8("将 source_node 的 signal_name 信号连接到 target_node 的 "
                            "target_method 方法。支持 CONNECT_DEFERRED(1)、CONNECT_ONESHOT(2) 等标志。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("源节点路径（发出信号的节点）");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要连接的信号名称");
            props["signal_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标节点路径（接收信号的节点）");
            props["target_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标节点上的方法名");
            props["target_method"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String::utf8("连接标志（1=DEFERRED, 2=ONESHOT, 4=PERSIST, 8=REFERENCE_COUNTED）");
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
            return ToolResult::err("MISSING_ARG", String::utf8("signal_name 不能为空"));
        }
        if (target_method.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("target_method 不能为空"));
        }

        Node *source = resolve_node(ctx.root, path);
        if (!source) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("源节点未找到: ") + path);
        }

        Node *target = resolve_node(ctx.root, target_path);
        if (!target) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("目标节点未找到: ") + target_path);
        }

        Callable callable(target, target_method);
        Error err = source->connect(signal_name, callable, (uint32_t)flags);
        if (err != OK) {
            String msg = String::utf8("连接信号失败，错误码: ") + itos((int)err);
            return ToolResult::err("CONNECT_FAILED", msg);
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
