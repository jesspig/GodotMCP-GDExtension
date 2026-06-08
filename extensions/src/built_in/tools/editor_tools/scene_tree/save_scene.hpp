// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SaveSceneTool : public ITool {
public:
    String name() const override { return "save_scene"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("保存当前编辑的场景到 res:// 路径");
    }
    String description() const override {
        return String::utf8("将当前打开的场景保存到磁盘。path 留空则保存到原 .tscn 路径。"
                            "如果场景从未保存且 path 为空，返回错误。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("res:// 路径（留空 = 保存到原路径，必须以 .tscn/.scn 结尾）");
            props["path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", String::utf8("EditorInterface 不可用"));
        }

        if (path.is_empty()) {
            if (ctx.root->get_scene_file_path().is_empty()) {
                return ToolResult::err("NO_PATH",
                    String::utf8("场景未保存过且未提供 path 参数"));
            }
            godot::Error err = ei->save_scene();
            if (err != godot::OK) {
                return ToolResult::err("SAVE_FAILED",
                    String::utf8("保存失败，错误码: ") + String::num_int64((int64_t)err));
            }
            path = ctx.root->get_scene_file_path();
        } else {
            if (!path.ends_with(".tscn") && !path.ends_with(".scn")) {
                return ToolResult::err("BAD_EXTENSION",
                    String::utf8("路径必须以 .tscn 或 .scn 结尾"));
            }
            if (!ensure_parent_dir(path)) {
                return ToolResult::err("MKDIR_FAILED",
                    String::utf8("无法创建父目录: ") + path);
            }
            ei->save_scene_as(path);
            notify_file_changed(path);
        }

        Dictionary data;
        data["path"] = path;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
