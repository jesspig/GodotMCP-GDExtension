#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

class SchemaBuilder {
private:
    Dictionary props_;
    Array required_;

public:
    SchemaBuilder &prop(const String &name, const String &type, const String &description) {
        Dictionary p;
        p["type"] = type;
        p["description"] = description;
        props_[name] = p;
        return *this;
    }

    SchemaBuilder &prop(const String &name, const String &type, const String &description, const Variant &default_value) {
        Dictionary p;
        p["type"] = type;
        p["description"] = description;
        p["default"] = default_value;
        props_[name] = p;
        return *this;
    }

    SchemaBuilder &required(const Array &names) {
        required_ = names;
        return *this;
    }

    Dictionary build() const {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props_;
        if (required_.size() > 0) {
            s["required"] = required_;
        }
        return s;
    }

    static Dictionary empty() {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }
};

} // namespace godot_mcp
