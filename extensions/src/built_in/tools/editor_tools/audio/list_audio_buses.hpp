
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/audio_server.hpp>

namespace godot_mcp {

class ListAudioBusesTool : public ITool {
public:
    String name() const override { return "list_audio_buses"; }
    String category() const override { return "editor_tools/audio"; }
    String brief() const override {
        return "List all audio buses and their properties";
    }
    String description() const override {
        return "Returns a list of all audio buses from the AudioServer singleton, "
               "including name, solo, mute, bypass, volume_db, effect count, and send target.";
    }
    Dictionary build_input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        godot::AudioServer *as = godot::AudioServer::get_singleton();
        if (!as) {
            return ToolResult::err("NO_AUDIO_SERVER", "AudioServer not available");
        }

        int32_t bus_count = as->get_bus_count();
        Array buses;

        for (int32_t i = 0; i < bus_count; i++) {
            Dictionary bus;
            bus["index"] = static_cast<int64_t>(i);
            bus["name"] = as->get_bus_name(i);
            bus["solo"] = as->is_bus_solo(i);
            bus["mute"] = as->is_bus_mute(i);
            bus["bypass"] = as->is_bus_bypassing_effects(i);
            bus["volume_db"] = as->get_bus_volume_db(i);
            bus["effect_count"] = static_cast<int64_t>(as->get_bus_effect_count(i));
            bus["send"] = String(as->get_bus_send(i));
            buses.append(bus);
        }

        Dictionary data;
        data["buses"] = buses;
        data["bus_count"] = static_cast<int64_t>(bus_count);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
