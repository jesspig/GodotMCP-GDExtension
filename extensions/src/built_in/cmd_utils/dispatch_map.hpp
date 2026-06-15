#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <utility>

namespace godot_mcp {

template <typename K, typename V, std::size_t N>
class DispatchMap {
public:
    constexpr DispatchMap(std::array<std::pair<K, V>, N> entries)
        : entries_(std::move(entries)) {}

    const V* find(const K& key) const noexcept {
        for (const auto& kv : entries_) {
            if (kv.first == key) return &kv.second;
        }
        return nullptr;
    }

    constexpr std::size_t size() const noexcept { return N; }
    constexpr bool empty() const noexcept { return N == 0; }

    bool validate() const noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = i + 1; j < N; ++j) {
                if (entries_[i].first == entries_[j].first) return false;
            }
        }
        return true;
    }

private:
    std::array<std::pair<K, V>, N> entries_;
};

template <typename K, typename V, typename... Pairs>
constexpr auto make_dispatch_map(Pairs&&... pairs) {
    return DispatchMap<K, V, sizeof...(Pairs)>{
        std::array<std::pair<K, V>, sizeof...(Pairs)>{{ std::forward<Pairs>(pairs)... }}
    };
}

} // namespace godot_mcp
