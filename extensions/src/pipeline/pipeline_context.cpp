#include "pipeline_context.hpp"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <string_view>

namespace godot_mcp {
namespace pipeline {

using namespace godot;

void PipelineContext::record_step(const godot::String &id, const StepResult &r) {
    std::lock_guard<std::mutex> lock(mtx_);
    step_results_[id] = r;
}

std::optional<StepResult> PipelineContext::get_step(const godot::String &id) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = step_results_.find(id);
    if (it != step_results_.end()) {
        return it->value;
    }
    return std::nullopt;
}

bool PipelineContext::eval_when(const WhenClause &c) const {
    auto sr = get_step(c.step_id);
    if (!sr.has_value()) {
        return false;
    }
    bool condition = false;
    if (c.field == "passed") {
        condition = (sr->status == StepStatus::Passed);
    } else if (c.field == "failed") {
        condition = (sr->status == StepStatus::Failed);
    } else if (c.field == "skipped") {
        condition = (sr->status == StepStatus::Skipped);
    }
    return condition == c.expected;
}

namespace {

godot::Variant resolve_path(const godot::Dictionary &root, const godot::String &path) {
    if (path.is_empty()) {
        return root;
    }
    godot::Variant current = root;
    godot::String remaining = path;
    while (!remaining.is_empty()) {
        if (current.get_type() != godot::Variant::DICTIONARY) {
            return godot::Variant();
        }
        godot::Dictionary d = current;
        int dot = static_cast<int>(remaining.find("."));
        godot::String key;
        if (dot != -1) {
            key = remaining.substr(0, dot);
            remaining = remaining.substr(dot + 1);
        } else {
            key = remaining;
            remaining = "";
        }
        current = d.get(key, godot::Variant());
    }
    return current;
}

bool parse_template_ref(const godot::String &s, godot::String &out_ref_id, godot::String &out_field, godot::String &out_path) {
    if (!s.begins_with("${") || !s.ends_with("}")) {
        return false;
    }
    godot::String inner = s.substr(2, s.length() - 3);
    const godot::Array parts = inner.split(".");
    if (parts.size() < 2) {
        return false;
    }
    if (parts[0] == "vars" && parts.size() >= 2) {
        out_ref_id = "vars";
        out_field = "";
        godot::String path_buf;
        for (int i = 1; i < static_cast<int>(parts.size()); ++i) {
            if (i > 1) path_buf += ".";
            path_buf += godot::String(parts[i]);
        }
        out_path = path_buf;
        return true;
    }
    if (parts.size() < 3 || parts[0] != "steps") {
        return false;
    }
    out_ref_id = parts[1];
    if (parts.size() >= 3) {
        out_field = parts[2];
        godot::String path_buf;
        for (int i = 3; i < static_cast<int>(parts.size()); ++i) {
            if (i > 3) path_buf += ".";
            path_buf += godot::String(parts[i]);
        }
        out_path = path_buf;
    }
    return true;
}

godot::Variant expand_variant(const godot::Variant &val, const PipelineContext &ctx) {
    switch (val.get_type()) {
        case godot::Variant::STRING: {
            godot::String s = val;
            godot::String ref_id, ref_field, ref_path;
            if (parse_template_ref(s, ref_id, ref_field, ref_path)) {
                if (ref_id == "vars") {
                    const godot::Dictionary &vars = ctx.get_workflow_vars();
                    godot::Variant resolved = resolve_path(vars, ref_path);
                    if (resolved.get_type() == godot::Variant::NIL) {
                        return godot::String("[ERROR: unknown var '") + ref_path + "']";
                    }
                    return resolved;
                }
                auto sr = ctx.get_step(ref_id);
                if (!sr.has_value()) {
                    return godot::String("[ERROR: unknown step '") + ref_id + "']";
                }
                if (ref_field == "status") {
                    switch (sr->status) {
                        case StepStatus::Passed: return godot::String("passed");
                        case StepStatus::Failed: return godot::String("failed");
                        case StepStatus::Skipped: return godot::String("skipped");
                        default: return godot::String("pending");
                    }
                }
                if (ref_path.is_empty()) {
                    return sr->raw_result;
                }
                return resolve_path(sr->raw_result, ref_path);
            }
            if (s.find("${") != -1) {
                int start = static_cast<int>(s.find("${"));
                int end = static_cast<int>(s.find("}", static_cast<int64_t>(start)));
                if (start != -1 && end != -1 && end > start) {
                    godot::String inner = s.substr(start + 2, end - start - 2);
                    const godot::Array parts = inner.split(".");
                    if (parts.size() >= 2 && parts[0] == "vars") {
                        godot::String path_buf;
                        for (int i = 1; i < static_cast<int>(parts.size()); ++i) {
                            if (i > 1) path_buf += ".";
                            path_buf += godot::String(parts[i]);
                        }
                        const godot::Dictionary &vars = ctx.get_workflow_vars();
                        godot::Variant resolved = resolve_path(vars, path_buf);
                        godot::String resolved_str = godot::JSON::stringify(resolved);
                        if (resolved_str.length() >= 2 &&
                            resolved_str[0] == '"' &&
                            resolved_str[resolved_str.length() - 1] == '"') {
                            resolved_str = resolved_str.substr(1, resolved_str.length() - 2);
                        }
                        return s.substr(0, start) + resolved_str + s.substr(end + 1);
                    }
                    if (parts.size() >= 3 && parts[0] == "steps") {
                        godot::String ref_id2 = parts[1];
                        auto sr = ctx.get_step(ref_id2);
                        if (sr.has_value() && parts.size() >= 4 && parts[2] == "result") {
                            godot::String path_buf;
                            for (int i = 3; i < static_cast<int>(parts.size()); ++i) {
                                if (i > 3) path_buf += ".";
                                path_buf += godot::String(parts[i]);
                            }
                            godot::Variant resolved = resolve_path(sr->raw_result, path_buf);
                            godot::String resolved_str = godot::JSON::stringify(resolved);
                            if (resolved_str.length() >= 2 &&
                                resolved_str[0] == '"' &&
                                resolved_str[resolved_str.length() - 1] == '"') {
                                resolved_str = resolved_str.substr(1, resolved_str.length() - 2);
                            }
                            return s.substr(0, start) + resolved_str + s.substr(end + 1);
                        }
                    }
                }
            }
            return s;
        }
        case godot::Variant::DICTIONARY: {
            godot::Dictionary d = val;
            godot::Dictionary result;
            const godot::Array keys = d.keys();
            for (int i = 0; i < keys.size(); ++i) {
                result[keys[i]] = expand_variant(d[keys[i]], ctx);
            }
            return result;
        }
        case godot::Variant::ARRAY: {
            godot::Array a = val;
            godot::Array result;
            for (int i = 0; i < a.size(); ++i) {
                result.push_back(expand_variant(a[i], ctx));
            }
            return result;
        }
        default:
            return val;
    }
}

} // anonymous namespace

godot::Dictionary PipelineContext::expand_templates(const godot::Dictionary &args) const {
    godot::Dictionary result;
    const godot::Array keys = args.keys();
    for (int i = 0; i < keys.size(); ++i) {
        result[keys[i]] = expand_variant(args[keys[i]], *this);
    }
    return result;
}

} // namespace pipeline
} // namespace godot_mcp