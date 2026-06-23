
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/cmd_utils/undo_stack.hpp"
#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/classes/time.hpp>

namespace godot_mcp {

class UndoTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const noexcept override { return "undo"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return String("Undo the last reversible tool operation"); }
    String category_description() const override { return String("Meta tools and system information queries"); }
    String description() const override {
        return String("Reverses the last tool operation by replaying its reverse arguments. "
                      "Only operations that registered undo parameters are reversible. "
                      "The undone operation is moved to the redo stack for potential redo.");
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
        if (!g_undo_manager->can_undo()) {
            return ToolResult::err("NOTHING_TO_UNDO", "Nothing to undo");
        }
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }

        const UndoRecord *rec = g_undo_manager->peek_undo();
        if (!rec) {
            return ToolResult::err("NOTHING_TO_UNDO", "Nothing to undo");
        }

        // Save record data before pop (pop invalidates the pointer)
        String tool_name = rec->tool_name;
        String description = rec->description;
        Dictionary reverse_args = rec->reverse_args;

        // Execute the reverse operation by calling the tool
        Dictionary result = reg_->execute(tool_name, reverse_args);
        if (result.has("success") && static_cast<bool>(result["success"])) {
            // Only pop on success
            g_undo_manager->pop_undo();
            Dictionary data;
            data["undone"] = tool_name;
            data["description"] = description;
            data["remaining_undos"] = static_cast<int64_t>(g_undo_manager->undo_count());
            data["redo_available"] = g_undo_manager->can_redo();
            return ToolResult::ok(data);
        }
        // On failure, discard the undo record (reverse operation failed)
        g_undo_manager->discard_undo();
        return ToolResult::err("UNDO_FAILED",
            String("Reverse operation for '") + tool_name + String("' failed"));
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
