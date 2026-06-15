#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <functional>

namespace godot_mcp {

class McpLogger {
public:
    struct LogEntry {
        godot::String timestamp;    // ISO 8601
        godot::String tool_name;
        bool success = false;
        godot::Dictionary args;
        godot::Dictionary result;
        double duration_ms = 0.0;
    };

    using LogCallback = std::function<void(const LogEntry &)>;

    void set_max_entries(int max);
    int max_entries() const;

    void set_log_dir(const godot::String &dir);
    godot::String log_dir() const;

    void set_log_callback(LogCallback cb);
    void append(const LogEntry &entry);
    const godot::Vector<LogEntry> &entries() const;
    ~McpLogger() {
        if (!pending_entries_.is_empty()) {
            flush();
        }
    }
    void clear();
    void rotate(int keep_days = 7);
    void ensure_log_dir();

private:
    godot::Vector<LogEntry> entries_;
    int max_entries_ = 500;
    godot::String log_dir_ = "res://.mcp_logs";
    godot::String current_log_file_;
    LogCallback callback_;
    godot::Vector<LogEntry> pending_entries_;
    static constexpr int kBatchSize = 10;

    godot::String log_filename() const;
    void write_to_jsonl(const LogEntry &entry);
    void flush();
    void rotate_files(int keep_days);
};

} // namespace godot_mcp
