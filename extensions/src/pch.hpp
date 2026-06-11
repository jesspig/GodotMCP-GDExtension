#ifndef GODOTMCP_PCH_HPP
#define GODOTMCP_PCH_HPP

// Precompiled headers — stable, frequently-used headers compiled once.
// Reduces build times ~30-50% on MSVC by avoiding redundant header parsing.

// Standard library
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <utility>
#include <algorithm>
#include <optional>
#include <array>
#include <set>
#include <type_traits>

// Godot core types (parsed once, reused across all TUs)
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/char_string.hpp>

// Expose godot types in all TUs at global scope.
// Tool headers (@tool register) use unqualified String / Dictionary / Variant
// etc. Without this, generated_registration.cpp (in a separate unity batch
// from register_types.cpp) fails to resolve them.
// PCH propagates this to every translation unit automatically.
using namespace godot;

#endif // GODOTMCP_PCH_HPP
