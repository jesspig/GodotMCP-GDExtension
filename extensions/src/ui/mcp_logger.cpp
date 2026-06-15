// =====================================================================
// mcp_logger.cpp — MCP 日志系统：内存存储 + JSONL 写入 + 自动轮转
// =====================================================================

#include "mcp_logger.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot_mcp {

// ---------------------------------------------------------------------
// 属性访问器
// ---------------------------------------------------------------------

void McpLogger::set_max_entries(int max) {
    max_entries_ = max > 0 ? max : 500;
    while (entries_.size() > max_entries_) {
        entries_.pop_front();
    }
}

int McpLogger::max_entries() const {
    return max_entries_;
}

void McpLogger::set_log_dir(const godot::String &dir) {
    log_dir_ = dir;
    current_log_file_ = "";
}

godot::String McpLogger::log_dir() const {
    return log_dir_;
}

void McpLogger::set_log_callback(LogCallback cb) {
    callback_ = std::move(cb);
}

// ---------------------------------------------------------------------
// 核心操作
// ---------------------------------------------------------------------

void McpLogger::append(const LogEntry &entry) {
    entries_.push_back(entry);
    while (entries_.size() > max_entries_) {
        entries_.pop_front();
    }
    pending_entries_.push_back(entry);
    if (pending_entries_.size() >= kBatchSize) {
        flush();
    }
    if (callback_) {
        callback_(entry);
    }
}

const std::deque<McpLogger::LogEntry> &McpLogger::entries() const {
    return entries_;
}

void McpLogger::clear() {
    entries_.clear();
    pending_entries_.clear();
}

// ---------------------------------------------------------------------
// 目录管理
// ---------------------------------------------------------------------

void McpLogger::ensure_log_dir() {
    godot::Ref<godot::DirAccess> da = godot::DirAccess::open(log_dir_);
    if (da.is_null()) {
        godot::Error err = godot::DirAccess::make_dir_recursive_absolute(log_dir_);
        if (err != godot::OK) {
            godot::UtilityFunctions::push_error("[McpLogger] Failed to create log dir: ", log_dir_);
        }
    }
}

// ---------------------------------------------------------------------
// 日志文件名生成
// ---------------------------------------------------------------------

godot::String McpLogger::log_filename() const {
    godot::Dictionary dt = godot::Time::get_singleton()->get_datetime_dict_from_system();
    int y = static_cast<int>(dt["year"]);
    int mo = static_cast<int>(dt["month"]);
    int d = static_cast<int>(dt["day"]);
    int h = static_cast<int>(dt["hour"]);
    int mi = static_cast<int>(dt["minute"]);
    int s = static_cast<int>(dt["second"]);
    return godot::String("mcp_")
        + godot::String::num_int64(y)
        + (mo < 10 ? "0" : "") + godot::String::num_int64(mo)
        + (d < 10 ? "0" : "") + godot::String::num_int64(d) + "_"
        + (h < 10 ? "0" : "") + godot::String::num_int64(h)
        + (mi < 10 ? "0" : "") + godot::String::num_int64(mi)
        + (s < 10 ? "0" : "") + godot::String::num_int64(s)
        + ".jsonl";
}

// ---------------------------------------------------------------------
// JSONL 写入
// ---------------------------------------------------------------------

godot::Ref<godot::FileAccess> McpLogger::ensure_log_file_opened() {
    godot::Ref<godot::FileAccess> f;
    if (godot::FileAccess::file_exists(current_log_file_)) {
        f = godot::FileAccess::open(current_log_file_, godot::FileAccess::READ_WRITE);
        if (f.is_valid()) f->seek_end();
    } else {
        f = godot::FileAccess::open(current_log_file_, godot::FileAccess::WRITE);
    }
    return f;
}

void McpLogger::write_to_jsonl(const LogEntry &entry) {
    ensure_log_dir();
    if (current_log_file_.is_empty()) {
        current_log_file_ = log_dir_.path_join(log_filename());
    }

    godot::Dictionary json_entry;
    json_entry["timestamp"] = entry.timestamp;
    json_entry["tool"] = entry.tool_name;
    json_entry["success"] = entry.success;
    json_entry["args"] = entry.args;
    json_entry["result"] = entry.result;
    json_entry["duration_ms"] = entry.duration_ms;

    godot::String line = godot::JSON::stringify(json_entry);

    godot::Ref<godot::FileAccess> f = ensure_log_file_opened();
    if (f.is_valid()) {
        f->store_string(line + "\n");
        f->close();
    } else {
        godot::UtilityFunctions::push_error(
            "[McpLogger] Failed to open log file: ", current_log_file_);
    }
}

// ---------------------------------------------------------------------
// 批量 flush
// ---------------------------------------------------------------------

void McpLogger::flush() {
    if (pending_entries_.empty()) return;

    if (current_log_file_.is_empty()) {
        current_log_file_ = log_dir_.path_join(log_filename());
    }

    ensure_log_dir();

    godot::Ref<godot::FileAccess> f = ensure_log_file_opened();

    if (f.is_valid()) {
        godot::String combined;
        for (const LogEntry &entry : pending_entries_) {
            godot::Dictionary json_entry;
            json_entry["timestamp"] = entry.timestamp;
            json_entry["tool"] = entry.tool_name;
            json_entry["success"] = entry.success;
            json_entry["args"] = entry.args;
            json_entry["result"] = entry.result;
            json_entry["duration_ms"] = entry.duration_ms;
            combined += godot::JSON::stringify(json_entry);
            combined += "\n";
        }
        godot::PackedByteArray buf = combined.to_utf8_buffer();
        f->store_buffer(buf);
        f->close();
    } else {
        godot::UtilityFunctions::push_error(
            "[McpLogger] Failed to open log file during flush: ", current_log_file_);
    }

    pending_entries_.clear();
}

// ---------------------------------------------------------------------
// 日志轮转
// ---------------------------------------------------------------------

void McpLogger::rotate(int keep_days) {
    flush();
    rotate_files(keep_days);
}

void McpLogger::rotate_files(int keep_days) {
    godot::Ref<godot::DirAccess> da = godot::DirAccess::open(log_dir_);
    if (da.is_null()) return;

    godot::Dictionary now_dt = godot::Time::get_singleton()->get_datetime_dict_from_system();
    int64_t now_unix = godot::Time::get_singleton()->get_unix_time_from_datetime_dict(now_dt);

    da->list_dir_begin();
    while (true) {
        godot::String name = da->get_next();
        if (name.is_empty()) break;
        if (da->current_is_dir()) continue;
        if (!name.begins_with("mcp_") || !name.ends_with(".jsonl")) continue;

        // 解析 mcp_YYYYMMDD_HHMMSS.jsonl
        if (name.length() < 24) continue;
        godot::String date_part = name.substr(4, 8); // YYYYMMDD
        int year = static_cast<int>(date_part.substr(0, 4).to_int());
        int month = static_cast<int>(date_part.substr(4, 2).to_int());
        int day = static_cast<int>(date_part.substr(6, 2).to_int());

        godot::Dictionary file_dt;
        file_dt["year"] = year;
        file_dt["month"] = month;
        file_dt["day"] = day;
        int64_t file_unix = godot::Time::get_singleton()->get_unix_time_from_datetime_dict(file_dt);
        int64_t diff_seconds = now_unix - file_unix;
        int diff = static_cast<int>(diff_seconds / 86400);

        if (diff > keep_days) {
            godot::String path = log_dir_.path_join(name);
            godot::Error err = da->remove(path);
            if (err == godot::OK) {
                godot::UtilityFunctions::print("[McpLogger] Rotated old log: ", name);
            }
        }
    }
    da->list_dir_end();
}

} // namespace godot_mcp
