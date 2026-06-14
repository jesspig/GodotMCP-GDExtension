#pragma once

#include "tool_base.hpp"
#include "server/registry/handler_registry.hpp"
#include <godot_cpp/variant/callable.hpp>
#include <functional>

namespace godot_mcp {

class IToolAdapter : public ITool {
public:
    IToolAdapter(
        const godot::String &name,
        const godot::String &category,
        const godot::String &brief,
        const godot::String &description,
        const godot::Dictionary &input_schema,
        CommandFn fn,
        bool is_meta = false,
        bool needs_scene = false,
        bool needs_node = false,
        bool supports_undo = false,
        bool is_destructive = false);

    IToolAdapter(
        const godot::String &name,
        const godot::String &category,
        const godot::String &brief,
        const godot::String &description,
        const godot::Dictionary &input_schema,
        const godot::Callable &callable,
        bool is_meta = false,
        bool needs_scene = false,
        bool needs_node = false,
        bool supports_undo = false,
        bool is_destructive = false);

    godot::String name() const override { return name_; }
    godot::String category() const override { return category_; }
    godot::String brief() const override { return brief_; }
    godot::String description() const override { return description_; }
    godot::Dictionary input_schema() const override { return input_schema_; }
    bool is_meta() const override { return is_meta_; }
    bool needs_scene() const override { return needs_scene_; }
    bool needs_node() const override { return needs_node_; }
    bool supports_undo() const override { return supports_undo_; }
    bool is_destructive() const override { return is_destructive_; }
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

protected:
    godot::Dictionary execute_impl(const ToolContext &ctx) override;

private:
    godot::String name_;
    godot::String category_;
    godot::String brief_;
    godot::String description_;
    godot::Dictionary input_schema_;
    bool is_meta_ = false;
    bool needs_scene_ = false;
    bool needs_node_ = false;
    bool supports_undo_ = false;
    CommandFn fn_;
    godot::Callable callable_;
    bool use_callable_ = false;
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
