#include "game_bridge.hpp"
#include "logging.hpp"

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
// Forward declaration - defined in cmd_utils_json.cpp, no editor dependency
Variant json_to_variant(const Variant &jv);

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
    OS *os = OS::get_singleton();
    if (!os) return 9601;
    const String raw = os->get_environment("GODOT_MCP_BRIDGE_PORT");
    if (raw.is_empty()) return 9601;
    int64_t parsed = raw.to_int();
    if (parsed < 1 || parsed > 65535) return 9601;
    return static_cast<int>(parsed);
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
    read_buf_.clear();
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
        read_buf_.clear();
        log_info("game_bridge", "Client connected");
    }
}

void GameBridgeNode::read_clients() {
    if (!client_.is_valid()) return;

    client_->poll();
    StreamPeerSocket::Status s = client_->get_status();
    if (s == StreamPeerSocket::STATUS_ERROR || s == StreamPeerSocket::STATUS_NONE) {
        client_.unref();
        read_buf_.clear();
        log_info("game_bridge", "Client disconnected");
        return;
    }

    int avail = client_->get_available_bytes();
    if (avail <= 0) return;

    Array chunk = client_->get_partial_data(avail);
    if (static_cast<int>(chunk[0]) != OK) {
        client_->disconnect_from_host();
        client_.unref();
        read_buf_.clear();
        return;
    }
    PackedByteArray data = chunk[1];
    read_buf_.append_array(data);

    if (read_buf_.size() > BUFFER_LIMIT) {
        // Sanity: drop connection if buffer overflows
        Dictionary err;
        err["ok"] = false;
        err["error"] = String("Message body too large");
        send_response(client_, err);
        client_->disconnect_from_host();
        client_.unref();
        read_buf_.clear();
        return;
    }

    // Parse the buffer directly as JSON (single-pass: accumulate until valid)
    String text = read_buf_.get_string_from_utf8();
    if (text.is_empty()) return; // partial UTF-8, wait for more data

    Ref<JSON> json;
    json.instantiate();
    Error parse_err = json->parse(text);
    if (parse_err != OK) {
        // Incomplete - wait for more data
        return;
    }
    Variant msg = json->get_data();

    // Calculate consumed bytes and keep any remaining data for next message
    int consumed = text.utf8().size();
    if (consumed > 0 && consumed < read_buf_.size()) {
        PackedByteArray remaining;
        remaining.resize(read_buf_.size() - consumed);
        memcpy(remaining.ptrw(), read_buf_.ptr() + consumed, remaining.size());
        read_buf_ = remaining;
    } else {
        read_buf_.clear();
    }

    if (msg.get_type() != Variant::DICTIONARY) {
        Dictionary err;
        err["ok"] = false;
        err["error"] = String("Message must be a JSON object");
        send_response(client_, err);
        return;
    }

    Dictionary msg_dict = msg;
    String cmd = msg_dict.get("cmd", "");
    Dictionary params = msg_dict.get("params", Dictionary());
    Variant id_v = msg_dict.get("id", Variant());
    int id = static_cast<int>((id_v.get_type() == Variant::FLOAT ? static_cast<double>(id_v ): static_cast<int64_t>(id_v)));

    Dictionary result = dispatch(cmd, params);
    result["id"] = id;
    send_response(client_, result);
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
    int max_depth = params.get("max_depth", -1);
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
    for (int i = 0; i < prop_list.size(); i++) {
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
    node->set(property, json_to_variant(params.get("value", Variant())));
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
    for (int i = 0; i < args.size(); i++) {
        converted_args[i] = json_to_variant(args[i]);
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

    // Named keys
    if (upper == "ENTER" || upper == "RETURN")    return KEY_ENTER;
    if (upper == "SPACE")                          return KEY_SPACE;
    if (upper == "ESCAPE" || upper == "ESC")       return KEY_ESCAPE;
    if (upper == "TAB")                            return KEY_TAB;
    if (upper == "BACKSPACE")                      return KEY_BACKSPACE;
    if (upper == "SHIFT")                          return KEY_SHIFT;
    if (upper == "CONTROL" || upper == "CTRL")     return KEY_CTRL;
    if (upper == "ALT")                            return KEY_ALT;
    if (upper == "META" || upper == "COMMAND")     return KEY_META;
    if (upper == "UP")                             return KEY_UP;
    if (upper == "DOWN")                           return KEY_DOWN;
    if (upper == "LEFT")                           return KEY_LEFT;
    if (upper == "RIGHT")                          return KEY_RIGHT;
    if (upper == "DELETE")                         return KEY_DELETE;
    if (upper == "HOME")                           return KEY_HOME;
    if (upper == "END")                            return KEY_END;
    if (upper == "PAGEUP" || upper == "PAGE_UP")   return KEY_PAGEUP;
    if (upper == "PAGEDOWN" || upper == "PAGE_DOWN") return KEY_PAGEDOWN;
    if (upper == "INSERT" || upper == "INS")       return KEY_INSERT;
    if (upper == "F1")                             return KEY_F1;
    if (upper == "F2")                             return KEY_F2;
    if (upper == "F3")                             return KEY_F3;
    if (upper == "F4")                             return KEY_F4;
    if (upper == "F5")                             return KEY_F5;
    if (upper == "F6")                             return KEY_F6;
    if (upper == "F7")                             return KEY_F7;
    if (upper == "F8")                             return KEY_F8;
    if (upper == "F9")                             return KEY_F9;
    if (upper == "F10")                            return KEY_F10;
    if (upper == "F11")                            return KEY_F11;
    if (upper == "F12")                            return KEY_F12;

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

Dictionary GameBridgeNode::node_to_dict(Node *node, int max_depth, int depth) {
    Dictionary d;
    if (!node) return d;

    d["name"] = node->get_name();
    d["type"] = node->get_class();
    d["path"] = node->get_path();

    if (max_depth < 0 || depth < max_depth) {
        Array children;
        int count = node->get_child_count();
        for (int i = 0; i < count; i++) {
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
