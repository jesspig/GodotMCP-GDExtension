#include "scene_diff.hpp"
#include "scene_diff_detail.hpp"

namespace godot_mcp::scene_diff {

Dictionary NodeChange::to_dict() const {
    Dictionary d;
    d["change_type"] = static_cast<int64_t>(type);
    d["node_path"] = node_path;
    d["node_data"] = node_data;
    if (type == ADDED) {
        d["operation"] = String("added");
    } else if (type == DELETED) {
        d["operation"] = String("deleted");
    } else if (type == MODIFIED) {
        d["operation"] = String("modified");
    } else if (type == REPARENTED) {
        d["operation"] = String("reparented");
        d["old_parent"] = old_parent;
        d["new_parent"] = new_parent;
    }
    return d;
}

int DiffResult::total_changes() const {
    return static_cast<int>(property_changes.size() + node_changes.size());
}

bool DiffResult::has_changes() const {
    return property_changes.size() > 0 || node_changes.size() > 0;
}

Dictionary DiffResult::to_dict() const {
    Array changes;
    for (int64_t i = 0; i < property_changes.size(); i++) {
        const auto &pc = property_changes[i];
        Dictionary d;
        d["change_type"] = String("property");
        d["node_path"] = pc.node_path;
        d["property"] = pc.property;
        d["old_value"] = pc.old_value;
        d["new_value"] = pc.new_value;
        changes.append(d);
    }
    for (int64_t i = 0; i < node_changes.size(); i++) {
        changes.append(node_changes[i].to_dict());
    }
    Dictionary result;
    result["changes"] = changes;
    result["total"] = total_changes();
    result["has_changes"] = has_changes();
    return result;
}

DiffResult SceneDiff::compute(Node *original_root, Node *current_root) {
    DiffResult result;

    if (!original_root || !current_root) {
        return result;
    }

    HashMap<String, Node*> orig_nodes;
    HashMap<String, Node*> curr_nodes;

    scene_diff_detail::collect_by_path(original_root, orig_nodes, "");
    scene_diff_detail::collect_by_path(current_root, curr_nodes, "");

    for (const auto &kv : orig_nodes) {
        if (!curr_nodes.has(kv.key)) {
            NodeChange nc;
            nc.type = NodeChange::DELETED;
            nc.node_path = kv.key;
            Dictionary data;
            data["type"] = kv.value->get_class();
            data["name"] = kv.value->get_name();
            nc.node_data = data;
            result.node_changes.push_back(nc);
        }
    }

    for (const auto &kv : curr_nodes) {
        if (!orig_nodes.has(kv.key)) {
            NodeChange nc;
            nc.type = NodeChange::ADDED;
            nc.node_path = kv.key;
            Dictionary data;
            data["type"] = kv.value->get_class();
            data["name"] = kv.value->get_name();
            nc.node_data = data;
            result.node_changes.push_back(nc);
        }
    }

    for (const auto &kv : curr_nodes) {
        if (!orig_nodes.has(kv.key)) continue;

        Node *orig = orig_nodes[kv.key];
        Node *curr = kv.value;

        Array prop_list = curr->get_property_list();
        for (int64_t i = 0; i < prop_list.size(); i++) {
            Dictionary pd = prop_list[i];
            String name = pd.get("name", "");

            if (scene_diff_detail::should_skip_property(name)) continue;

            Variant orig_val = orig->get(name);
            Variant curr_val = curr->get(name);

            if (orig_val != curr_val) {
                PropertyChange pc;
                pc.node_path = kv.key;
                pc.property = name;
                pc.old_value = orig_val;
                pc.new_value = curr_val;
                result.property_changes.push_back(pc);
            }
        }
    }

    return result;
}

} // namespace godot_mcp::scene_diff
