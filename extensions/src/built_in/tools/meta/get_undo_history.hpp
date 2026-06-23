
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/cmd_utils/undo_stack.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tool_base.hpp"

namespace godot_mcp {

class GetUndoHistoryTool : public ITool {
public:
    String name() const noexcept override { return "get_undo_history"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return String("Return undo/redo stack contents"); }
    String category_description() const override { return String("Meta tools and system information queries"); }
    String description() const override {
        return String("Returns the current undo and redo stack contents without modifying them. "
                      "Useful for checking what operations are reversible before committing to undo.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("max_items", "integer", "Maximum items per stack (default 50)", (int64_t)50)
            .build();
    }
    bool is_meta() const noexcept override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!g_undo_manager) {
            return ToolResult::err("NO_UNDO_MANAGER", "UndoManager not initialized");
        }
        const int64_t max_items = args_int(ctx.args, "max_items", 50);

        Dictionary data;
        data["undo_stack"] = g_undo_manager->get_undo_history(max_items);
        data["redo_stack"] = g_undo_manager->get_redo_history(max_items);
        data["undo_count"] = static_cast<int64_t>(g_undo_manager->undo_count());
        data["redo_count"] = static_cast<int64_t>(g_undo_manager->redo_count());
        data["max_steps"] = static_cast<int64_t>(g_undo_manager->max_steps());
        data["can_undo"] = g_undo_manager->can_undo();
        data["can_redo"] = g_undo_manager->can_redo();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
