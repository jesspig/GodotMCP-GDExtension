#include "search_engine.hpp"
#include "handler_registry.hpp"

#include <algorithm>
#include <vector>

using namespace godot;

namespace godot_mcp {

PackedStringArray SearchEngine::tokenize(const String &text) {
    String clean = text.to_lower();
    clean = clean.replace("_", " ").replace("/", " ").replace(".", " ").replace("-", " ");
    PackedStringArray raw = clean.split(" ", false);
    PackedStringArray result;
    for (int64_t i = 0; i < raw.size(); ++i) {
        String t = raw[i].strip_edges();
        if (!t.is_empty()) {
            result.push_back(t);
        }
    }
    return result;
}

void SearchEngine::record_tool_call(const String &name) {
    auto it = call_frequency_.find(name);
    if (it != call_frequency_.end()) {
        it->value++;
    } else {
        call_frequency_[name] = 1;
    }
}

Array SearchEngine::search_tools(const String &query, const String &category, int limit) const {
    if (query.is_empty() || !tool_info_) return Array();

    const String q = query.to_lower().strip_edges();
    HashMap<String, int> best_weight;

    for (const auto &[tool_name, info] : *tool_info_) {
        if (!category.is_empty() && !info.category.begins_with(category)) continue;

        const String name_lower = tool_name.to_lower();

        if (name_lower == q) {
            best_weight[tool_name] = 1000;
            continue;
        }

        if (name_lower.begins_with(q)) {
            best_weight[tool_name] = 500;
            continue;
        }

        PackedStringArray tokens = tokenize(tool_name + String(" ") + info.brief + String(" ") + info.description);
        bool token_matched = false;
        for (int64_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].find(q) >= 0) {
                best_weight[tool_name] = 200;
                token_matched = true;
                break;
            }
        }
        if (token_matched) continue;

        if (info.description.to_lower().find(q) >= 0) {
            best_weight[tool_name] = 50;
        }
    }

    struct ScoredTool {
        String name;
        int weight;
        int freq;
    };
    std::vector<ScoredTool> scored;
    for (const auto &[tool_name, weight] : best_weight) {
        ScoredTool st;
        st.name = tool_name;
        st.weight = weight;
        auto fit = call_frequency_.find(tool_name);
        st.freq = static_cast<int>((fit != call_frequency_.end()) ? fit->value : 0);
        scored.push_back(st);
    }

    std::sort(scored.begin(), scored.end(), [](const ScoredTool &a, const ScoredTool &b) {
        if (a.freq != b.freq) return a.freq > b.freq;
        return a.weight > b.weight;
    });

    Array result;
    for (int64_t i = 0; i < static_cast<int>(scored.size()) && i < limit; ++i) {
        Dictionary entry;
        entry["name"] = scored[i].name;
        auto info_it = tool_info_->find(scored[i].name);
        if (info_it != tool_info_->end()) {
            entry["brief"] = info_it->value.brief;
            entry["category"] = info_it->value.category;
            entry["description"] = info_it->value.description;
        }
        entry["frequency"] = scored[i].freq;
        result.push_back(entry);
    }

    return result;
}

Array SearchEngine::get_search_suggestions(const String &prefix, int limit) const {
    if (prefix.is_empty() || !tool_info_) return Array();

    const String p = prefix.to_lower().strip_edges();
    HashMap<String, int> candidates;

    for (const auto &[tool_name, info] : *tool_info_) {
        if (tool_name.to_lower().begins_with(p)) {
            auto fit = call_frequency_.find(tool_name);
            candidates[tool_name] = static_cast<int>((fit != call_frequency_.end()) ? fit->value : 0);
        }
    }

    for (const auto &[tool_name, info] : *tool_info_) {
        if (candidates.has(tool_name)) continue;
        PackedStringArray tokens = tokenize(tool_name + String(" ") + info.brief + String(" ") + info.description);
        for (int64_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].begins_with(p)) {
                auto fit = call_frequency_.find(tool_name);
                candidates[tool_name] = static_cast<int>((fit != call_frequency_.end()) ? fit->value : 0);
                break;
            }
        }
    }

    struct SuggEntry {
        String name;
        int freq;
    };
    std::vector<SuggEntry> sorted;
    for (const auto &[tool_name, freq] : candidates) {
        sorted.push_back({tool_name, freq});
    }
    std::sort(sorted.begin(), sorted.end(), [](const SuggEntry &a, const SuggEntry &b) {
        return a.freq > b.freq;
    });

    Array result;
    for (int64_t i = 0; i < sorted.size() && i < limit; ++i) {
        result.push_back(sorted[i].name);
    }
    return result;
}

Vector<String> SearchEngine::get_most_called_tools(int count) const {
    if (call_frequency_.is_empty()) return {};

    std::vector<std::pair<String, int64_t>> freq_vec;
    for (const auto &[tool_name, freq] : call_frequency_) {
        freq_vec.push_back({tool_name, freq});
    }
    std::sort(freq_vec.begin(), freq_vec.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    Vector<String> result;
    for (size_t i = 0; i < freq_vec.size() && i < static_cast<size_t>(count); ++i) {
        result.push_back(freq_vec[i].first);
    }
    return result;
}

} // namespace godot_mcp
