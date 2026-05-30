// commands/find.cpp — find_nodes_by_name/type/group/script

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/array.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary push_match(Node *n, Node *root) {
    Dictionary r;
    r["name"] = n->get_name();
    r["type"] = n->get_class();
    r["path"] = relative_path(root, n);
    return r;
}

void collect_by_name(Node *n, const String &pattern, Array &out, int64_t max, Node *root) {
    if (out.size() >= max) return;
    if (n->get_name().contains(pattern)) out.append(push_match(n, root));
    for (int64_t i = 0; i < n->get_child_count(); i++) {
        Node *c = Object::cast_to<Node>(n->get_child(i));
        if (c) collect_by_name(c, pattern, out, max, root);
    }
}
void collect_by_type(Node *n, const String &cls, Array &out, int64_t max, Node *root) {
    if (out.size() >= max) return;
    if (n->is_class(cls)) out.append(push_match(n, root));
    for (int64_t i = 0; i < n->get_child_count(); i++) {
        Node *c = Object::cast_to<Node>(n->get_child(i));
        if (c) collect_by_type(c, cls, out, max, root);
    }
}
void collect_by_group(Node *n, const String &group, Array &out, int64_t max, Node *root) {
    if (out.size() >= max) return;
    if (n->is_in_group(group)) out.append(push_match(n, root));
    for (int64_t i = 0; i < n->get_child_count(); i++) {
        Node *c = Object::cast_to<Node>(n->get_child(i));
        if (c) collect_by_group(c, group, out, max, root);
    }
}
void collect_by_script(Node *n, const String &script_path, Array &out, int64_t max, Node *root) {
    if (out.size() >= max) return;
    Variant sv = n->get("script");
    Ref<Script> script = sv;
    if (script.is_valid()) {
        String sp = script->get_path();
        if (sp.contains(script_path)) out.append(push_match(n, root));
    }
    for (int64_t i = 0; i < n->get_child_count(); i++) {
        Node *c = Object::cast_to<Node>(n->get_child(i));
        if (c) collect_by_script(c, script_path, out, max, root);
    }
}

Dictionary cmd_find_by_name(const Dictionary &a) {
    String pattern = args_string(a, "pattern");
    if (pattern.is_empty()) return make_error("missing 'pattern'");
    int64_t max = args_int(a, "max_results", 100);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Array out; collect_by_name(root, pattern, out, max, root);
    Dictionary r; r["matches"] = out; r["count"] = (int64_t)out.size(); r["truncated"] = out.size() >= max; return r;
}
Dictionary cmd_find_by_type(const Dictionary &a) {
    String cls = args_string(a, "node_class");
    if (cls.is_empty()) return make_error("missing 'node_class'");
    int64_t max = args_int(a, "max_results", 100);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Array out; collect_by_type(root, cls, out, max, root);
    Dictionary r; r["matches"] = out; r["count"] = (int64_t)out.size(); r["truncated"] = out.size() >= max; return r;
}
Dictionary cmd_find_by_group(const Dictionary &a) {
    String group = args_string(a, "group");
    if (group.is_empty()) return make_error("missing 'group'");
    int64_t max = args_int(a, "max_results", 100);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Array out; collect_by_group(root, group, out, max, root);
    Dictionary r; r["matches"] = out; r["count"] = (int64_t)out.size(); r["truncated"] = out.size() >= max; return r;
}
Dictionary cmd_find_by_script(const Dictionary &a) {
    String sp = args_string(a, "script_path");
    if (sp.is_empty()) return make_error("missing 'script_path'");
    int64_t max = args_int(a, "max_results", 100);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Array out; collect_by_script(root, sp, out, max, root);
    Dictionary r; r["matches"] = out; r["count"] = (int64_t)out.size(); r["truncated"] = out.size() >= max; return r;
}
} // namespace

void register_find(HandlerRegistry &reg) {
    reg.register_tool("find_nodes_by_name", cmd_find_by_name);
    reg.register_tool("find_nodes_by_type", cmd_find_by_type);
    reg.register_tool("find_nodes_by_group", cmd_find_by_group);
    reg.register_tool("find_nodes_by_script", cmd_find_by_script);
}
} // namespace godot_mcp