#include "game_bridge.hpp"
#include "built_in/cmd_utils.hpp"
#include "logging.hpp"

#include <algorithm>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/marshalls.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace godot_mcp {
// Forward declaration - defined in cmd_utils_json.cpp, no editor dependency.
Variant json_to_variant(const Variant &jv, int depth);

// -----------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------

GameBridgeNode::GameBridgeNode() {
    port_ = read_port();
}

GameBridgeNode::~GameBridgeNode() = default;

void GameBridgeNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_self_add"), &GameBridgeNode::_self_add);
}

int GameBridgeNode::read_port() {
    return read_port_from_env("GODOT_MCP_BRIDGE_PORT", 9601);
}

void GameBridgeNode::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    set_process(true);
    connect_to_editor();
}

void GameBridgeNode::_exit_tree() {
    disconnect_from_editor();
}

void GameBridgeNode::_process(double) {
    if (!connection_.is_valid()) {
        // Retry connection every 2 seconds
        const uint64_t now = Time::get_singleton()->get_ticks_msec();
        if (now > connect_retry_msec_) {
            connect_to_editor();
            connect_retry_msec_ = now + 2000;
        }
        return;
    }

    connection_->poll();
    StreamPeerTCP::Status s = connection_->get_status();
    if (s == StreamPeerTCP::STATUS_ERROR || s == StreamPeerTCP::STATUS_NONE) {
        log_warn("game_bridge", "Editor bridge disconnected");
        connection_.unref();
        reset_read_state_internal();
        return;
    }

    if (s == StreamPeerTCP::STATUS_CONNECTED) {
        read_from_editor();
    }
}

void GameBridgeNode::_self_add() {
    if (Engine::get_singleton()->is_editor_hint()) return;
    if (is_inside_tree()) return;

    Node *root = get_scene_root();
    if (!root) {
        call_deferred("_self_add");
        return;
    }

    root->add_child(this);
    log_info("game_bridge", String("GameBridgeNode added to scene tree, connecting to editor bridge :") +
                                    String::num_int64(port_));
}

// -----------------------------------------------------------------------
// TCP Client: connect to editor's RuntimeBridgeServer
// -----------------------------------------------------------------------

void GameBridgeNode::connect_to_editor() {
    if (connection_.is_valid()) return;

    connection_.instantiate();
    Error err = connection_->connect_to_host("127.0.0.1", port_);
    if (err != OK) {
        connection_.unref();
        return;
    }
    log_info("game_bridge", String("Connecting to editor bridge 127.0.0.1:") + String::num_int64(port_));
}

void GameBridgeNode::disconnect_from_editor() {
    if (connection_.is_valid()) {
        connection_->disconnect_from_host();
        connection_.unref();
    }
    reset_read_state_internal();
}

// -----------------------------------------------------------------------
// Read from editor (same framing protocol, now reads from connection_)
// -----------------------------------------------------------------------

void GameBridgeNode::read_from_editor() {
    if (!connection_.is_valid()) return;

    int64_t avail = connection_->get_available_bytes();
    if (avail <= 0) return;

    Array chunk = connection_->get_partial_data(static_cast<int>(avail));
    if (static_cast<int64_t>(chunk[0]) != OK) {
        connection_.unref();
        reset_read_state_internal();
        return;
    }
    PackedByteArray data = chunk[1];
    read_buf_.append_array(data);

    if (read_buf_.size() > BUFFER_LIMIT) {
        log_warn("game_bridge", "Buffer overflow, resetting");
        reset_read_state_internal();
        return;
    }

    while (true) {
        if (read_offset_ + 4 > read_buf_.size()) break;

        const uint8_t *p = read_buf_.ptr() + read_offset_;
        const uint32_t msg_len = (static_cast<uint32_t>(p[0]) << 24) |
                                 (static_cast<uint32_t>(p[1]) << 16) |
                                 (static_cast<uint32_t>(p[2]) << 8) |
                                  static_cast<uint32_t>(p[3]);

        if (msg_len == 0 || msg_len > MAX_MESSAGE_SIZE) {
            log_warn("game_bridge", "Invalid message length, resetting buffer");
            reset_read_state_internal();
            return;
        }

        if (read_offset_ + 4 + static_cast<int64_t>(msg_len) > read_buf_.size()) break;

        read_offset_ += 4;

        String text;
        text.parse_utf8(reinterpret_cast<const char *>(read_buf_.ptr() + read_offset_),
                        static_cast<int>(msg_len));
        read_offset_ += static_cast<int64_t>(msg_len);

        Ref<JSON> json;
        json.instantiate();
        if (json->parse(text) != OK) {
            log_warn("game_bridge", "Invalid JSON message");
            continue;
        }

        Variant msg = json->get_data();
        if (msg.get_type() != Variant::DICTIONARY) {
            log_warn("game_bridge", "Message is not a dictionary");
            continue;
        }

        Dictionary msg_dict = msg;
        String cmd = msg_dict.get("cmd", "");
        Dictionary params = msg_dict.get("params", Dictionary());
        Variant id = msg_dict.get("id", Variant());

        Dictionary result = dispatch(cmd, params);
        result["id"] = id;
        send_response(result);
    }

    if (read_offset_ > 0) {
        const int remaining = read_buf_.size() - static_cast<int>(read_offset_);
        if (remaining > 0) {
            PackedByteArray new_buf;
            new_buf.resize(remaining);
            std::copy_n(read_buf_.ptr() + read_offset_, remaining, new_buf.ptrw());
            read_buf_ = new_buf;
        } else {
            read_buf_.clear();
        }
        read_offset_ = 0;
    }
}

void GameBridgeNode::reset_read_state_internal() {
    read_buf_.clear();
    read_offset_ = 0;
}

void GameBridgeNode::send_response(const Dictionary &msg) {
    if (!connection_.is_valid()) return;

    PackedByteArray json_bytes = JSON::stringify(msg).to_utf8_buffer();
    if (json_bytes.size() > MAX_MESSAGE_SIZE) {
        log_error("game_bridge", "Response too large");
        return;
    }

    const uint32_t len = static_cast<uint32_t>(json_bytes.size());
    PackedByteArray frame;
    frame.resize(4 + json_bytes.size());
    auto *p = frame.ptrw();
    p[0] = static_cast<uint8_t>((len >> 24) & 0xFF);
    p[1] = static_cast<uint8_t>((len >> 16) & 0xFF);
    p[2] = static_cast<uint8_t>((len >> 8) & 0xFF);
    p[3] = static_cast<uint8_t>(len & 0xFF);
    std::copy_n(json_bytes.ptr(), json_bytes.size(), p + 4);

    Error err = connection_->put_data(frame);
    if (err != OK) {
        log_error("game_bridge", String("send_response put_data failed: err=") + String::num_int64(err));
    }
}

// -----------------------------------------------------------------------
// Scene tree helpers
// -----------------------------------------------------------------------

Node *GameBridgeNode::get_scene_root() {
    SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (!st || !st->get_root()) return nullptr;
    return st->get_root();
}

// -----------------------------------------------------------------------
// Command dispatch
// -----------------------------------------------------------------------

Dictionary GameBridgeNode::dispatch(const String &cmd, const Dictionary &params) {
    if (cmd == "get_scene_tree")    return handle_get_scene_tree(params);
    if (cmd == "get_property")      return handle_get_property(params);
    if (cmd == "set_property")      return handle_set_property(params);
    if (cmd == "call_method")       return handle_call_method(params);
    if (cmd == "screenshot")        return handle_screenshot(params);
    if (cmd == "simulate_input")    return handle_simulate_input(params);
    if (cmd == "set_pause")         return handle_set_pause(params);

    Dictionary r;
    r["ok"] = false;
    r["error"] = String("Unknown command: ") + cmd;
    return r;
}

// -----------------------------------------------------------------------
// Command handlers (unchanged from v1)
// -----------------------------------------------------------------------

Dictionary GameBridgeNode::handle_get_scene_tree(const Dictionary &params) {
    Node *root = get_scene_root();
    if (!root) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Scene tree not available");
        return r;
    }
    int64_t max_depth = static_cast<int64_t>(params.get("max_depth", -1));
    Dictionary result = node_to_dict(root, max_depth, 0);
    Dictionary r;
    r["ok"] = true;
    r["data"] = result;
    return r;
}

Dictionary GameBridgeNode::handle_get_property(const Dictionary &params) {
    String node_path = params.get("node_path", "");
    String property = params.get("property", "");
    if (node_path.is_empty() || property.is_empty()) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Parameters node_path and property must not be empty");
        return r;
    }
    Node *root = get_scene_root();
    if (!root) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Scene tree not available");
        return r;
    }
    Node *node = root->get_node<godot::Node>(NodePath(node_path));
    if (!node) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Node not found: ") + node_path;
        return r;
    }

    bool has_prop = false;
    Array prop_list = node->get_property_list();
    for (int64_t i = 0; i < prop_list.size(); i++) {
        Dictionary pd = prop_list[i];
        if (pd.get("name", "") == property) {
            has_prop = true;
            break;
        }
    }
    if (!has_prop) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Property not found: ") + property;
        return r;
    }

    Variant value = node->get(property);
    Dictionary r;
    r["ok"] = true;
    r["data"] = value;
    return r;
}

Dictionary GameBridgeNode::handle_set_property(const Dictionary &params) {
    String node_path = params.get("node_path", "");
    String property = params.get("property", "");
    if (node_path.is_empty() || property.is_empty()) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Parameters node_path and property must not be empty");
        return r;
    }
    Node *root = get_scene_root();
    if (!root) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Scene tree not available");
        return r;
    }
    Node *node = root->get_node<godot::Node>(NodePath(node_path));
    if (!node) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Node not found: ") + node_path;
        return r;
    }
    node->set(property, json_to_variant(params.get("value", Variant()), 0));
    Dictionary r;
    r["ok"] = true;
    r["data"] = node->get(property);
    return r;
}

Dictionary GameBridgeNode::handle_call_method(const Dictionary &params) {
    String node_path = params.get("node_path", "");
    String method = params.get("method", "");
    if (node_path.is_empty() || method.is_empty()) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Parameters node_path and method must not be empty");
        return r;
    }
    Node *root = get_scene_root();
    if (!root) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Scene tree not available");
        return r;
    }
    Node *node = root->get_node<godot::Node>(NodePath(node_path));
    if (!node) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Node not found: ") + node_path;
        return r;
    }
    if (!node->has_method(method)) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Method '") + method + String("' not found on node ") + node_path;
        return r;
    }
    Array args = params.get("args", Array());
    Array converted_args;
    converted_args.resize(args.size());
    for (int64_t i = 0; i < args.size(); i++) {
        converted_args[i] = json_to_variant(args[i], 0);
    }
    Variant result = node->callv(method, converted_args);
    Dictionary r;
    r["ok"] = true;
    r["data"] = result;
    r["return_type"] = Variant::get_type_name(result.get_type());
    return r;
}

Dictionary GameBridgeNode::handle_screenshot(const Dictionary &params) {
    String format = params.get("format", "png");
    int64_t max_w = params.get("max_width", static_cast<int64_t>(0));
    int64_t max_h = params.get("max_height", static_cast<int64_t>(0));
    double quality = params.get("quality", 0.85);

    Viewport *vp = get_viewport();
    if (!vp) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Viewport not available");
        return r;
    }
    Ref<Image> img = vp->get_texture()->get_image();
    if (img.is_null()) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Unable to get viewport image");
        return r;
    }

    // Resize if max dimensions specified
    int64_t w = img->get_width();
    int64_t h = img->get_height();
    if (max_w > 0 || max_h > 0) {
        int64_t target_w = max_w > 0 ? max_w : w;
        int64_t target_h = max_h > 0 ? max_h : h;
        float scale = std::min(static_cast<float>(target_w) / static_cast<float>(w),
                               static_cast<float>(target_h) / static_cast<float>(h));
        if (scale < 1.0f) {
            int64_t new_w = std::max(static_cast<int64_t>(1), static_cast<int64_t>(w * scale));
            int64_t new_h = std::max(static_cast<int64_t>(1), static_cast<int64_t>(h * scale));
            img->resize(static_cast<int>(new_w), static_cast<int>(new_h), Image::INTERPOLATE_BILINEAR);
        }
    }

    PackedByteArray buf;
    if (format == "jpg" || format == "jpeg") {
        float q = static_cast<float>(std::clamp(quality, 0.0, 1.0));
        buf = img->save_jpg_to_buffer(q);
    } else {
        buf = img->save_png_to_buffer();
    }
    String b64 = Marshalls::get_singleton()->raw_to_base64(buf);
    String mime = (format == "jpg" || format == "jpeg") ? String("image/jpeg") : String("image/png");

    Dictionary data;
    data["mime"] = mime;
    data["data"] = b64;
    data["size"] = buf.size();
    data["width"] = img->get_width();
    data["height"] = img->get_height();

    Dictionary r;
    r["ok"] = true;
    r["data"] = data;
    return r;
}

static Key key_from_string(const String &name) {
    String upper = name.to_upper();

    if (upper.length() == 1) {
        char32_t c = upper[0];
        if (c >= 'A' && c <= 'Z') return static_cast<Key>((KEY_A + (c - 'A')));
        if (c >= '0' && c <= '9') return static_cast<Key>((KEY_0 + (c - '0')));
    }

    struct KeyEntry { const char *name; Key key; };
    static constexpr KeyEntry kEntries[] = {
        {"ENTER", KEY_ENTER}, {"RETURN", KEY_ENTER},
        {"SPACE", KEY_SPACE},
        {"ESCAPE", KEY_ESCAPE}, {"ESC", KEY_ESCAPE},
        {"TAB", KEY_TAB},
        {"BACKSPACE", KEY_BACKSPACE},
        {"SHIFT", KEY_SHIFT},
        {"CONTROL", KEY_CTRL}, {"CTRL", KEY_CTRL},
        {"ALT", KEY_ALT},
        {"META", KEY_META}, {"COMMAND", KEY_META},
        {"UP", KEY_UP}, {"DOWN", KEY_DOWN}, {"LEFT", KEY_LEFT}, {"RIGHT", KEY_RIGHT},
        {"DELETE", KEY_DELETE}, {"HOME", KEY_HOME}, {"END", KEY_END},
        {"PAGEUP", KEY_PAGEUP}, {"PAGE_UP", KEY_PAGEUP},
        {"PAGEDOWN", KEY_PAGEDOWN}, {"PAGE_DOWN", KEY_PAGEDOWN},
        {"INSERT", KEY_INSERT}, {"INS", KEY_INSERT},
        {"F1", KEY_F1}, {"F2", KEY_F2}, {"F3", KEY_F3}, {"F4", KEY_F4},
        {"F5", KEY_F5}, {"F6", KEY_F6}, {"F7", KEY_F7}, {"F8", KEY_F8},
        {"F9", KEY_F9}, {"F10", KEY_F10}, {"F11", KEY_F11}, {"F12", KEY_F12},
    };

    for (const auto &entry : kEntries) {
        if (upper == String(entry.name)) return entry.key;
    }
    return KEY_NONE;
}

static MouseButton mouse_button_from_string(const String &name) {
    String upper = name.to_upper();
    if (upper == "LEFT")       return MOUSE_BUTTON_LEFT;
    if (upper == "RIGHT")      return MOUSE_BUTTON_RIGHT;
    if (upper == "MIDDLE")     return MOUSE_BUTTON_MIDDLE;
    if (upper == "WHEEL_UP")   return MOUSE_BUTTON_WHEEL_UP;
    if (upper == "WHEEL_DOWN") return MOUSE_BUTTON_WHEEL_DOWN;
    return MOUSE_BUTTON_NONE;
}

Dictionary GameBridgeNode::handle_simulate_input(const Dictionary &params) {
    Array actions = params.get("actions", Array());
    Input *input = Input::get_singleton();
    if (!input) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Input singleton not available");
        return r;
    }

    int success_count = 0;
    int fail_count = 0;

    for (int i = 0; i < actions.size(); i++) {
        Dictionary act = actions[i];
        String type = act.get("type", "");

        if (type == "key") {
            Ref<InputEventKey> ev;
            ev.instantiate();
            ev->set_pressed(act.get("pressed", true));
            String key_name = act.get("key", "");
            ev->set_keycode(key_from_string(key_name));
            if (ev->get_keycode() == KEY_NONE && !key_name.is_empty()) {
                fail_count++;
                continue;
            }
            input->parse_input_event(ev);
            success_count++;

        } else if (type == "mouse_button") {
            Ref<InputEventMouseButton> ev;
            ev.instantiate();
            ev->set_pressed(act.get("pressed", true));
            ev->set_button_index(mouse_button_from_string(act.get("button", "left")));
            ev->set_position(Vector2(
                static_cast<float>(act.get("x", 0)),
                static_cast<float>(act.get("y", 0))));
            ev->set_global_position(ev->get_position());
            input->parse_input_event(ev);
            success_count++;

        } else if (type == "mouse_motion") {
            Ref<InputEventMouseMotion> ev;
            ev.instantiate();
            ev->set_position(Vector2(
                static_cast<float>(act.get("x", 0)),
                static_cast<float>(act.get("y", 0))));
            ev->set_global_position(ev->get_position());
            ev->set_relative(Vector2(
                static_cast<float>(act.get("dx", 0)),
                static_cast<float>(act.get("dy", 0))));
            input->parse_input_event(ev);
            success_count++;

        } else if (type == "action") {
            Ref<InputEventAction> ev;
            ev.instantiate();
            ev->set_action(act.get("action", ""));
            ev->set_pressed(act.get("pressed", true));
            ev->set_strength(static_cast<float>(act.get("strength", 1.0)));
            if (ev->get_action() == StringName()) {
                fail_count++;
                continue;
            }
            input->parse_input_event(ev);
            success_count++;

        } else {
            fail_count++;
        }
    }

    Dictionary data;
    data["success_count"] = success_count;
    data["fail_count"] = fail_count;

    Dictionary r;
    r["ok"] = fail_count == 0;
    if (fail_count > 0) {
        r["error"] = String::num_int64(fail_count) + " input event(s) failed";
    }
    r["data"] = data;
    return r;
}

Dictionary GameBridgeNode::handle_set_pause(const Dictionary &params) {
    SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (!st) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String("Scene tree not available");
        return r;
    }
    bool paused = params.get("paused", false);
    st->set_pause(paused);

    Dictionary data;
    data["paused"] = paused;

    Dictionary r;
    r["ok"] = true;
    r["data"] = data;
    return r;
}

// -----------------------------------------------------------------------
// Scene tree serialisation
// -----------------------------------------------------------------------

Dictionary GameBridgeNode::node_to_dict(Node *node, int64_t max_depth, int64_t depth) {
    Dictionary d;
    if (!node) return d;

    d["name"] = node->get_name();
    d["type"] = node->get_class();
    d["path"] = node->get_path();

    if (max_depth < 0 || depth < max_depth) {
        Array children;
        int64_t count = node->get_child_count();
        for (int64_t i = 0; i < count; i++) {
            Node *child = node->get_child(i);
            if (child) {
                children.push_back(node_to_dict(child, max_depth, depth + 1));
            }
        }
        if (children.size() > 0) {
            d["children"] = children;
        }
    }

    return d;
}

} // namespace godot_mcp
