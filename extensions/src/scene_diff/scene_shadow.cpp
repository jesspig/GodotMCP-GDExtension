#include "scene_shadow.hpp"
#include "scene_patcher.hpp"
#include "scene_diff_detail.hpp"

#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/core/memory.hpp>

using namespace godot;

namespace godot_mcp::scene_diff {

SceneShadow &get_global_shadow() {
    static SceneShadow instance;
    return instance;
}

Error SceneShadow::capture() {
    auto *ei = EditorInterface::get_singleton();
    if (!ei) return ERR_UNCONFIGURED;

    Node *root = ei->get_edited_scene_root();
    if (!root) return ERR_UNCONFIGURED;

    scene_path_ = root->get_scene_file_path();

    SceneSnapshot snapper;
    Error err = snapper.capture(root, snapshot_);
    if (err != OK) return err;

    HashMap<String, Node*> nodes;
    scene_diff_detail::collect_by_path(root, nodes, "");

    for (const auto &kv : nodes) {
        Dictionary props;
        Array prop_list = kv.value->get_property_list();
        for (int64_t i = 0; i < prop_list.size(); i++) {
            Dictionary pd = prop_list[i];
            String name = pd.get("name", "");
            if (scene_diff_detail::should_skip_property(name)) continue;
            props[name] = kv.value->get(name);
        }
        cached_properties_[kv.key] = props;
    }

    has_snapshot_ = true;
    return OK;
}

void SceneShadow::clear() {
    snapshot_ = Ref<PackedScene>();
    scene_path_ = String();
    has_snapshot_ = false;
    cached_properties_.clear();
}

DiffResult SceneShadow::compute_diff() const {
    if (!has_snapshot_) return DiffResult();

    Node *original = snapshot_->instantiate();
    if (!original) return DiffResult();

    auto *ei = EditorInterface::get_singleton();
    if (!ei) {
        memdelete(original);
        return DiffResult();
    }

    Node *current = ei->get_edited_scene_root();
    if (!current) {
        memdelete(original);
        return DiffResult();
    }

    DiffResult diff = SceneDiff::compute(original, current);
    memdelete(original);
    return diff;
}

int SceneShadow::apply() {
    DiffResult diff = compute_diff();
    if (!diff.has_changes()) return 0;

    auto *ei = EditorInterface::get_singleton();
    if (!ei) return 0;

    Node *root = ei->get_edited_scene_root();
    if (!root) return 0;

    auto *ur = get_undo_redo();
    if (!ur) return 0;

    ScenePatcher patcher;
    int count = patcher.apply(root, ur, diff);
    clear();
    return count;
}

void SceneShadow::rollback() {
    if (!has_snapshot_) return;

    auto *ei = EditorInterface::get_singleton();
    if (!ei) return;

    Node *current_root = ei->get_edited_scene_root();
    if (!current_root) return;

    ScenePatcher patcher;
    patcher.rollback(current_root, snapshot_);
    clear();
}

} // namespace godot_mcp::scene_diff
