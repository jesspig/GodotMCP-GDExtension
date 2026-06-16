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
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace godot_mcp {
// Forward declaration - defined in cmd_utils_json.cpp, no editor dependency.
// Keep this signature in sync with cmd_utils.hpp (the depth default lives in
// the header; redeclaring the default here would be an ODR violation, so omit it).
Variant json_to_variant(const Variant &jv, int depth);

static Dictionary make_error(const String &msg) {
    Dictionary r;
    r["ok"] = false;
    r["error"] = msg;
    return r;
}

static constexpr int64_t MAX_MESSAGE_SIZE = 65536;

static void reset_read_state(PackedByteArray &buf, String &text, int64_t &offset,
                              int &retries, int64_t &fail_offset, int64_t &consumed) {
    buf.clear();
    text = String();
    offset = 0;
    retries = 0;
    fail_offset = -1;
    consumed = 0;
}

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
        // Should never happen - only created in game process
        return;
    }
    set_process(true);
    start_server();
}

void GameBridgeNode::_exit_tree() {
    stop_server();
}

void GameBridgeNode::_process(double) {
    if (server_.is_valid()) {
        accept_clients();
        read_clients();
    }
}

void GameBridgeNode::_self_add() {
    if (Engine::get_singleton()->is_editor_hint()) return;
    if (is_inside_tree()) return;

    Node *root = get_scene_root();
    if (!root) {
        // Root not ready yet - try again next frame
        call_deferred("_self_add");
        return;
    }

    root->add_child(this);
    log_info("game_bridge", String("GameBridgeNode added to scene tree, port=") +
                                    String::num_int64(port_));
}

// -----------------------------------------------------------------------
// TCP Server
// -----------------------------------------------------------------------

void GameBridgeNode::start_server() {
    server_.instantiate();
    Error err = server_->listen(port_, "localhost");
    if (err != OK) {
        log_error("game_bridge", String("Failed to listen on :") + String::num_int64(port_));
        server_.unref();
        return;
    }
    log_info("game_bridge", String("GameBridge listening on :") + String::num_int64(port_));
}

void GameBridgeNode::stop_server() {
    if (client_.is_valid()) {
        client_->disconnect_from_host();
        client_.unref();
    }
    if (server_.is_valid()) {
        server_->stop();
        server_.unref();
    }
    reset_read_state_internal();
}

void GameBridgeNode::accept_clients() {
    if (client_.is_valid()) {
        // Already have a connected client
        return;
    }
    if (!server_->is_connection_available()) {
        return;
    }
    Ref<StreamPeerTCP> new_client = server_->take_connection();
    if (new_client.is_valid()) {
        new_client->set_no_delay(true);
        client_ = new_client;
        reset_read_state_internal();
        log_info("game_bridge", "Client connected");
    }
}

void GameBridgeNode::read_clients() {
    if (!client_.is_valid()) return;

    client_->poll();
    StreamPeerTCP::Status s = client_->get_status();
    if (s == StreamPeerTCP::STATUS_ERROR || s == StreamPeerTCP::STATUS_NONE) {
        client_.unref();
        reset_read_state_internal();
        log_info("game_bridge", "Client disconnected");
        return;
    }

    int64_t avail = client_->get_available_bytes();
    if (avail <= 0) return;

    Array chunk = client_->get_partial_data(avail);
    if (static_cast<int64_t>(chunk[0]) != OK) {
        client_->disconnect_from_host();
        client_.unref();
        reset_read_state_internal();
        return;
    }
    PackedByteArray data = chunk[1];
    read_buf_.append_array(data);

    if (read_buf_.size() > BUFFER_LIMIT) {
        Dictionary err = make_error("Message body too large");
        send_response(client_, err);
        client_->disconnect_from_host();
        client_.unref();
        reset_read_state_internal();
        return;
    }

    // Decode only newly received bytes and accumulate the decoded text
    int64_t new_bytes = read_buf_.size() - read_offset_;
    if (new_bytes > 0) {
        const uint8_t* raw = read_buf_.ptr() + read_offset_;
        String chunk_text;
        int64_t parse_len = (new_bytes > 65536) ? 65536 : new_bytes;
        Error utf8_err = chunk_text.parse_utf8((const char*)raw, static_cast<int>(parse_len));
        if (utf8_err != OK) {
            if (read_offset_ == utf8_fail_offset_) {
                utf8_retries_++;
            } else {
                utf8_retries_ = 0;
                utf8_fail_offset_ = read_offset_;
            }
            if (utf8_retries_ >= 3) {
                read_offset_ = read_buf_.size();
                utf8_retries_ = 0;
                utf8_fail_offset_ = -1;
                log_warn("game_bridge", "Discarding un-decodable UTF-8 data");
            }
            return;
        }
        utf8_retries_ = 0;
        utf8_fail_offset_ = -1;
        read_text_ += chunk_text;
        read_offset_ = read_buf_.size();
    }

    if (read_text_.is_empty()) return;

    // Process newline-delimited messages (fixes pipelined messages and JSON parse hang)
    while (true) {
        int64_t newline_idx = read_text_.find("\n");
        if (newline_idx == -1) {
            // Incomplete message – guard against runaway accumulation
            if (read_text_.length() > MAX_MESSAGE_SIZE) {
                log_warn("game_bridge", "Message too large, discarding buffer");
                reset_read_state_internal();
            }
            return;
        }

        String line = read_text_.substr(0, static_cast<int>(newline_idx));
        read_text_ = read_text_.substr(static_cast<int>(newline_idx) + 1);

        // All of read_buf_ has been decoded, safe to clear
        read_buf_.clear();
        read_offset_ = 0;

        if (line.is_empty()) continue;

        Ref<JSON> json;
        json.instantiate();
        if (json->parse(line) != OK) {
            log_warn("game_bridge", "Skipping unparseable JSON message");
            continue;
        }

        Variant msg = json->get_data();
        if (msg.get_type() != Variant::DICTIONARY) {
            log_warn("game_bridge", "Message must be a JSON object, skipping");
            continue;
        }

        Dictionary msg_dict = msg;
        String cmd = msg_dict.get("cmd", "");
        Dictionary params = msg_dict.get("params", Dictionary());
        Variant id = msg_dict.get("id", Variant());

        Dictionary result = dispatch(cmd, params);
        result["id"] = id;
        send_response(client_, result);
    }
}

void GameBridgeNode::reset_read_state_internal() {
    reset_read_state(read_buf_, read_text_, read_offset_,
                      utf8_retries_, utf8_fail_offset_, consumed_bytes_);
}

void GameBridgeNode::send_response(const Ref<StreamPeerTCP> &client, const Dictionary &msg) {
    if (!client.is_valid()) return;
    String json_str = JSON::stringify(msg);
    PackedByteArray data = json_str.to_utf8_buffer();
    data.push_back('\n');
    Error err = client->put_data(data);
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
// Command handlers
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
    return r;
}

Dictionary GameBridgeNode::handle_screenshot(const Dictionary &params) {
    String format = params.get("format", "png");
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
    PackedByteArray buf;
    if (format == "jpg" || format == "jpeg") {
        buf = img->save_jpg_to_buffer(0.85f);
    } else {
        buf = img->save_png_to_buffer();
    }
    String b64 = Marshalls::get_singleton()->raw_to_base64(buf);
    String mime = (format == "jpg" || format == "jpeg") ? String("image/jpeg") : String("image/png");

    Dictionary data;
    data["mime"] = mime;
    data["data"] = b64;
    data["size"] = buf.size();

    Dictionary r;
    r["ok"] = true;
    r["data"] = data;
    return r;
}

static Key key_from_string(const String &name) {
    String upper = name.to_upper();

    // Single letter A-Z
    if (upper.length() == 1) {
        char32_t c = upper[0];
        if (c >= 'A' && c <= 'Z') return static_cast<Key>((KEY_A + (c - 'A')));
        if (c >= '0' && c <= '9') return static_cast<Key>((KEY_0 + (c - '0')));
    }

    // Named keys via constexpr lookup table
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
