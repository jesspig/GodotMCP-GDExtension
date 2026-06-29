#pragma once

#include "built_in/tool_base.hpp"

namespace godot_mcp {

class RecordOperationsTool : public ITool {
public:
    String name() const noexcept override { return "record_operations"; }
    String category() const noexcept override { return "editor_tools/scene"; }
    String brief() const noexcept override {
        return "Start or stop recording tool operations for replay";
    }
    Dictionary build_input_schema() const override;
    bool needs_scene() const override { return false; }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override;
};

class ReplayOperationsTool : public ITool {
public:
    String name() const noexcept override { return "replay_operations"; }
    String category() const noexcept override { return "editor_tools/scene"; }
    String brief() const noexcept override {
        return "Replay a recorded sequence of tool operations from YAML";
    }
    Dictionary build_input_schema() const override;
    bool needs_scene() const override { return false; }
    bool is_destructive() const override { return true; }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override;
private:
    HandlerRegistry *registry_ = nullptr;
};

} // namespace godot_mcp
