
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/cmd_utils/undo_stack.hpp"
#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class RedoTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const noexcept override { return "redo"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return String("Redo the last undone operation"); }
    String category_description() const override { return String("Meta tools and system information queries"); }
    String description() const override {
        return String("Re-applies the last undone operation by replaying its forward arguments. "
                      "Only available after an undo has been performed.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder().build();
    }
    bool is_meta() const noexcept override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!g_undo_manager) {
            return ToolResult::err("NO_UNDO_MANAGER", "UndoManager not initialized");
        }
        if (!g_undo_manager->can_redo()) {
            return ToolResult::err("NOTHING_TO_REDO", "Nothing to redo");
        }
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }

        const UndoRecord *rec = g_undo_manager->peek_redo();
        if (!rec) {
            return ToolResult::err("NOTHING_TO_REDO", "Nothing to redo");
        }

        // Save record data before pop (pop invalidates the pointer)
        String tool_name = rec->tool_name;
        String description = rec->description;
        Dictionary forward_args = rec->forward_args;

        // Execute the forward operation by calling the tool
        Dictionary result = reg_->execute(tool_name, forward_args);
        if (result.has("success") && static_cast<bool>(result["success"])) {
            g_undo_manager->pop_redo();
            Dictionary data;
            data["redone"] = tool_name;
            data["description"] = description;
            data["remaining_redos"] = static_cast<int64_t>(g_undo_manager->redo_count());
            data["undo_available"] = g_undo_manager->can_undo();
            return ToolResult::ok(data);
        }
        // On failure, discard the redo record
        g_undo_manager->discard_redo();
        return ToolResult::err("REDO_FAILED",
            String("Forward operation for '") + tool_name + String("' failed"));
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
