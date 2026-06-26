// undo_stack.hpp — MCP self-managed undo/redo infrastructure
//
// Each successful tool call pushes an UndoRecord onto the undo stack.
// undo() replays reverse_args by calling the same tool implementation.
// Redo replays forward_args. Both stacks are cleared on push.
//
// Capacity is configurable via ProjectSettings "godot_mcp/undo_max_steps"
// (default 64, min 16, max 512). Configurable at startup via the
// Settings editor; changes take effect on next push/undo.
//
// NOT thread-safe — designed for single-threaded _process() model.

#pragma once

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstdint>
#include <deque>

namespace godot_mcp {

// A single undoable step.
// forward_args: the arguments that were used to call the tool.
// reverse_args: the arguments needed to undo the effect.
//               Empty dictionary means "cannot be automatically undone" (skip on undo).
struct UndoRecord {
    godot::String tool_name;
    godot::Dictionary forward_args;
    godot::Dictionary reverse_args;
    double timestamp;       // Time::get_singleton()->get_unix_time_from_system()
    godot::String description;  // Human-readable summary of what was done
};

class UndoManager {
public:
    UndoManager() = default;
    ~UndoManager() = default;

    // Push a record onto the undo stack. Clears the redo stack.
    // Returns false if the record has empty reverse_args (non-undoable).
    bool push(UndoRecord record) {
        if (record.reverse_args.is_empty()) {
            return false;
        }
        undo_stack_.push_back(std::move(record));
        while (static_cast<int>(undo_stack_.size()) > max_steps()) {
            undo_stack_.pop_front();
        }
        redo_stack_.clear();
        return true;
    }

    // Pop and return the next record to undo. Returns nullptr if empty.
    // The caller is responsible for executing reverse_args and calling
    // redo_push() with the record on success.
    const UndoRecord *peek_undo() const {
        if (undo_stack_.empty()) return nullptr;
        return &undo_stack_.back();
    }

    void pop_undo() {
        if (!undo_stack_.empty()) {
            redo_stack_.push_back(std::move(undo_stack_.back()));
            undo_stack_.pop_back();
        }
    }

    // Pop and return the next record to redo. Returns nullptr if empty.
    const UndoRecord *peek_redo() const {
        if (redo_stack_.empty()) return nullptr;
        return &redo_stack_.back();
    }

    void pop_redo() {
        if (!redo_stack_.empty()) {
            undo_stack_.push_back(std::move(redo_stack_.back()));
            redo_stack_.pop_back();
        }
    }

    // Discard the top redo record (on failure).
    void discard_redo() {
        if (!redo_stack_.empty()) {
            redo_stack_.pop_back();
        }
    }

    // Discard the top undo record (on failure).
    void discard_undo() {
        if (!undo_stack_.empty()) {
            undo_stack_.pop_back();
        }
    }

    // Return a snapshot of the undo history (oldest first).
    godot::Array get_undo_history(int64_t max_items = 50) const {
        godot::Array result;
        const int64_t count = std::min(static_cast<int64_t>(undo_stack_.size()), max_items);
        // Return most recent first
        for (int64_t i = 0; i < count; ++i) {
            const auto &rec = undo_stack_[undo_stack_.size() - 1 - static_cast<size_t>(i)];
            godot::Dictionary entry;
            entry["tool"] = rec.tool_name;
            entry["description"] = rec.description;
            entry["timestamp"] = rec.timestamp;
            result.append(entry);
        }
        return result;
    }

    // Return a snapshot of the redo history (oldest first).
    godot::Array get_redo_history(int64_t max_items = 50) const {
        godot::Array result;
        const int64_t count = std::min(static_cast<int64_t>(redo_stack_.size()), max_items);
        for (int64_t i = 0; i < count; ++i) {
            const auto &rec = redo_stack_[redo_stack_.size() - 1 - static_cast<size_t>(i)];
            godot::Dictionary entry;
            entry["tool"] = rec.tool_name;
            entry["description"] = rec.description;
            entry["timestamp"] = rec.timestamp;
            result.append(entry);
        }
        return result;
    }

    bool can_undo() const { return !undo_stack_.empty(); }
    bool can_redo() const { return !redo_stack_.empty(); }
    int undo_count() const { return static_cast<int>(undo_stack_.size()); }
    int redo_count() const { return static_cast<int>(redo_stack_.size()); }

    void clear() {
        undo_stack_.clear();
        redo_stack_.clear();
    }

    // Current max_steps from ProjectSettings.
    // Reads once per call; cached value expires after each push/undo/redo.
    int max_steps() const {
        auto *ps = godot::ProjectSettings::get_singleton();
        if (ps && ps->has_setting("godot_mcp/undo_max_steps")) {
            const int val = static_cast<int>(
                static_cast<int64_t>(ps->get_setting("godot_mcp/undo_max_steps")));
            if (val >= 16 && val <= 512) return val;
        }
        return 64;
    }

    // Register the setting with ProjectSettings (call once at startup).
    static void register_setting() {
        auto *ps = godot::ProjectSettings::get_singleton();
        if (!ps) return;
        if (!ps->has_setting("godot_mcp/undo_max_steps")) {
            ps->set_setting("godot_mcp/undo_max_steps", static_cast<int64_t>(64));
        }
        godot::Dictionary info;
        info["name"] = "godot_mcp/undo_max_steps";
        info["type"] = static_cast<int64_t>(godot::Variant::INT);
        info["hint"] = static_cast<int64_t>(godot::PROPERTY_HINT_RANGE);
        info["hint_string"] = "16,512,1";
        ps->add_property_info(info);
        ps->set_initial_value("godot_mcp/undo_max_steps", static_cast<int64_t>(64));
    }

private:
    std::deque<UndoRecord> undo_stack_;
    std::deque<UndoRecord> redo_stack_;
};

// Global singleton. Owned by McpEditorPlugin, freed on shutdown.
inline UndoManager *g_undo_manager = nullptr;

} // namespace godot_mcp
