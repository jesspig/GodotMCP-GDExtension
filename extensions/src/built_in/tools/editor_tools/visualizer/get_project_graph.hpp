
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetProjectGraphTool : public ITool {
public:
    String name() const noexcept override { return "get_project_graph"; }
    String category() const noexcept override { return "editor_tools/visualizer"; }
    String brief() const noexcept override { return "Get the project's scene dependency graph"; }
    String description() const override {
        return "Scans .tscn files in the project and analyzes scene dependencies. "
               "Returns a list of nodes (scene files) and edges (child/signal/resource dependencies). "
               "Also returns the structure of the currently open scene in the editor. Read-only operation with no side effects.";
    }

    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Maximum recursion depth";
            p["default"] = 3;
            props["max_depth"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Whether to include in-scene resource references";
            p["default"] = true;
            props["include_scene_resources"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        int64_t max_depth = args_int(ctx.args, "max_depth", 3);
        bool include_resources = args_bool(ctx.args, "include_scene_resources", true);

        Array nodes;
        Array edges;
        Dictionary visited_scenes;

        // Collect all .tscn files in project
        Array scene_files;
        walk_project_dir("res://", Array::make(".tscn"), false, 500, scene_files);

        // Build HashSet for O(1) scene lookups
        godot::HashSet<godot::String> scene_set;
        for (int64_t j = 0; j < scene_files.size(); j++) {
            scene_set.insert(godot::String(scene_files[j]));
        }

        // Build graph nodes for each scene file
        for (int64_t i = 0; i < scene_files.size(); i++) {
            String scene_path = scene_files[i];
            visited_scenes[scene_path] = true;

            Dictionary node_entry;
            node_entry["id"] = scene_path;
            node_entry["type"] = "scene";
            node_entry["name"] = _get_file_name(scene_path);
            node_entry["path"] = scene_path;
            nodes.append(node_entry);

            // Parse scene dependencies (limited by max_depth)
            if (i < max_depth || max_depth <= 0) {
                _analyze_scene_deps(scene_path, scene_path, edges, include_resources, 0, max_depth, visited_scenes, scene_files, scene_set);
            }
        }

        // Add current edited scene info
        auto *ei = godot::EditorInterface::get_singleton();
        String current_scene;
        if (ei) {
            Node *root = ei->get_edited_scene_root();
            if (root) {
                String scene_path = root->get_scene_file_path();
                if (!scene_path.is_empty())
                    current_scene = scene_path;

                // Walk current scene tree for node-level graph
                _walk_scene_tree(root, root, nodes, edges, include_resources);
            }
        }

        Dictionary data;
        data["nodes"] = nodes;
        data["edges"] = edges;
        data["scene_count"] = static_cast<int64_t>(scene_files.size());
        data["current_scene"] = current_scene;
        return ToolResult::ok(data);
    }

private:
    static String _get_file_name(const String &path) {
        int64_t last_slash = path.rfind("/");
        if (last_slash >= 0)
            return path.substr(last_slash + 1);
        return path;
    }

    static String _get_parent_path(const String &path) {
        int64_t last_slash = path.rfind("/");
        if (last_slash >= 0)
            return path.substr(0, last_slash);
        return "res://";
    }

    void _analyze_scene_deps(const String &scene_path, const String &from_id,
                             Array &edges, bool include_resources,
                             int64_t depth, int64_t max_depth,
                             Dictionary &visited, const Array &all_scenes,
                             const godot::HashSet<godot::String> &scene_set) const {
        if (max_depth > 0 && depth >= max_depth) return;

        godot::Ref<godot::PackedScene> packed = godot::ResourceLoader::get_singleton()->load(scene_path, "PackedScene");
        if (packed.is_null()) return;

        Node *temp = packed->instantiate();
        if (!temp) return;

        // Find child scene references (instanced scenes)
        _find_child_scenes(temp, from_id, edges, scene_set);

        // Find signal connections
        if (include_resources) {
            _find_signal_edges(temp, from_id, edges);
        }

        memdelete(temp);
    }

    void _find_child_scenes(Node *node, const String &from_id,
                            Array &edges, const godot::HashSet<godot::String> &scene_set) const {
        for (int64_t i = 0; i < node->get_child_count(); i++) {
            Node *child = node->get_child(i);
            if (!child) continue;

            // Check if child has an instanced scene
            String child_scene = child->get_scene_file_path();
            if (!child_scene.is_empty() && scene_set.has(child_scene)) {
                Dictionary edge;
                edge["from"] = from_id;
                edge["to"] = child_scene;
                edge["label"] = String(child->get_name());
                edge["type"] = "child";
                edges.append(edge);
            }

            _find_child_scenes(child, from_id, edges, scene_set);
        }
    }

    void _find_signal_edges(Node *node, const String &from_id,
                            Array &edges) const {
        // Use get_signal_connection_list to find signal connections
        Array connections = node->call("get_signal_connection_list");
        for (int64_t i = 0; i < connections.size(); i++) {
            Dictionary conn = connections[i];
            Dictionary edge;
            edge["from"] = from_id;
            edge["to"] = from_id;
            edge["label"] = String(conn.get("signal", "")) + " -> " + String(conn.get("method", ""));
            edge["type"] = "signal";
            edges.append(edge);
        }
    }

    void _walk_scene_tree(Node *node, Node *root,
                          Array &nodes, Array &edges,
                          bool include_resources) const {
        if (!node) return;

        String node_id = _build_node_id(root, node);
        String parent_id = _build_node_id(root, node->get_parent());

        Dictionary node_entry;
        node_entry["id"] = node_id;
        node_entry["type"] = node->get_class();
        node_entry["name"] = node->get_name();
        node_entry["path"] = root->get_path_to(node);
        nodes.append(node_entry);

        if (node != root) {
            Dictionary edge;
            edge["from"] = parent_id;
            edge["to"] = node_id;
            edge["label"] = String(node->get_name());
            edge["type"] = "child";
            edges.append(edge);
        }

        // Signal connections
        if (include_resources) {
            Array connections = node->call("get_signal_connection_list");
            for (int64_t i = 0; i < connections.size(); i++) {
                Dictionary conn = connections[i];
                Dictionary edge;
                edge["from"] = node_id;
                edge["to"] = node_id;
                edge["label"] = String(conn.get("signal", "")) + " -> " + String(conn.get("method", ""));
                edge["type"] = "signal";
                edges.append(edge);
            }
        }

        for (int64_t i = 0; i < node->get_child_count(); i++) {
            _walk_scene_tree(node->get_child(i), root, nodes, edges, include_resources);
        }
    }

    static String _build_node_id(Node *root, Node *node) {
        if (!node) return "";
        if (node == root) return ".";
        return root->get_path_to(node);
    }
};

} // namespace godot_mcp
