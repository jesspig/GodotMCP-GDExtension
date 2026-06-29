#include "scene_snapshot.hpp"

namespace godot_mcp::scene_diff {

Error SceneSnapshot::capture(Node *root, Ref<PackedScene> &out) {
    out.instantiate();
    return out->pack(root);
}

} // namespace godot_mcp::scene_diff
