
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class GetBestPracticesTool : public ITool {
public:
    String name() const override { return "get_best_practices"; }
    String category() const override { return "editor_tools/docs"; }
    String brief() const override {
        return "Return Godot best practices and coding guidelines";
    }
    String description() const override {
        return "Return curated best practices for Godot development "
               "across scene organization, scripting, performance, "
               "and UI design topics.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Topic: scene_organization, scripting, "
                               "performance, ui_design, or all (default)";
            props["topic"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }

protected:
    void add_practice(Array &list, const String &title,
                      const String &desc, const Array &examples) {
        Dictionary entry;
        entry["title"] = title;
        entry["description"] = desc;
        entry["examples"] = examples;
        list.append(entry);
    }

    Dictionary execute_impl(const ToolContext &ctx) override {
        String topic = args_string(ctx.args, "topic", "all");

        Array guidelines;

        if (topic == "scene_organization" || topic == "all") {
            Array ex1 = Array::make("Use /root/Game/Levels/Level1 instead of deep flat lists",
                                    "Group related nodes under a common parent",
                                    "Use @onready for frequent node references");
            add_practice(guidelines, "Organize Scene Tree Hierarchically",
                "Structure your scene tree with clear parent-child relationships. "
                "Avoid deeply nested structures (>5 levels) and keep siblings "
                "grouped by purpose.", ex1);

            Array ex2 = Array::make("res://scenes/levels/level_001.tscn",
                                    "res://scenes/characters/player/player.tscn",
                                    "res://scenes/shared/enemies/enemy_base.tscn");
            add_practice(guidelines, "Use Consistent Naming Conventions",
                "Use snake_case for file names and PascalCase for node names. "
                "Organize scenes into a clear directory structure: "
                "scenes/, scripts/, assets/, etc.", ex2);

            Array ex3 = Array::make("Use .tscn for reusable scenes",
                                    "Prefer instancing over duplicating nodes",
                                    "Use scene inheritance for variations");
            add_practice(guidelines, "Prefer Composition over Inheritance",
                "Build scenes by composing smaller, reusable scenes. "
                "Use scene inheritance only when you need a clear is-a "
                "relationship between scenes.", ex3);
        }

        if (topic == "scripting" || topic == "all") {
            Array ex1 = Array::make("var health: int = 100",
                                    "func take_damage(amount: int) -> void:",
                                    "Use typed arrays: Array[Node]");
            add_practice(guidelines, "Use Static Typing",
                "Godot supports optional static typing in GDScript 2.0. "
                "Always declare types for variables, function parameters, "
                "and return types to catch errors early and improve performance.", ex1);

            Array ex2 = Array::make("Use @export var speed: float = 300.0",
                                    "Use @onready var sprite: Sprite2D = $Sprite2D",
                                    "Group signals at the top of the script");
            add_practice(guidelines, "Use Annotations and Decorators",
                "Use @export for editable inspector properties, @onready to "
                "defer node initialization, @tool for editor scripts, "
                "and @warning_ignore to suppress specific warnings.", ex2);

            Array ex3 = Array::make("signal health_changed(new_value: int)",
                                    "Use .connect() with Callable",
                                    "Prefer one-shot signal connections");
            add_practice(guidelines, "Use Signals for Loose Coupling",
                "Signals decouple senders from receivers. Define signals "
                "at class level and connect them via the editor or in code. "
                "Avoid tight coupling through direct node references.", ex3);
        }

        if (topic == "performance" || topic == "all") {
            Array ex1 = Array::make("Use Object pooling for frequent spawns",
                                    "Prefer CanvasItem drawing over many Sprite2D nodes",
                                    "Use VisibilityNotifier2D/3D to cull off-screen nodes");
            add_practice(guidelines, "Optimize Rendering",
                "Minimize draw calls by using TextureAtlases, reducing "
                "overdraw, and using culling. For 2D, prefer "
                "CanvasItem drawing for many similar objects.", ex1);

            Array ex2 = Array::make("Use PhysicsServer2D/3D directly for many bodies",
                                    "Set sleep=on for static objects",
                                    "Reduce physics FPS if high precision is not needed");
            add_practice(guidelines, "Optimize Physics",
                "Use layers and masks to reduce collision checks. "
                "Set RigidBody sleep enabled. Consider lowering "
                "physics FPS for mobile/web targets.", ex2);

            Array ex3 = Array::make("Preload resources with preload()",
                                    "Use ResourceLoader.load() for background loading",
                                    "Free unused resources with ResourceQueue");
            add_practice(guidelines, "Manage Resources Efficiently",
                "Preload resources used throughout a scene. For large "
                "levels, use ResourceLoader.load() with background loading. "
                "Avoid loading all resources at game start.", ex3);
        }

        if (topic == "ui_design" || topic == "all") {
            Array ex1 = Array::make("Use Control nodes for responsive layout",
                                    "Use Container nodes (HBox, VBox, Grid) for auto-layout",
                                    "Anchor controls to edges for resize handling");
            add_practice(guidelines, "Use Containers for Responsive UI",
                "Godot's container system handles layout automatically. "
                "Use containers instead of absolute positioning to create "
                "UIs that adapt to different screen sizes.", ex1);

            Array ex2 = Array::make("Set minimum_size on controls",
                                    "Use theme constants for consistent spacing",
                                    "Use StyleBoxes for background styling");
            add_practice(guidelines, "Design for Multiple Resolutions",
                "Use the built-in theme system for consistent look. "
                "Set anchors and margins carefully. Test on multiple "
                "resolutions and aspect ratios.", ex2);

            Array ex3 = Array::make("Use Theme overrides sparingly",
                                    "Create custom Theme resources for reuse",
                                    "Use Theme Variation for small tweaks");
            add_practice(guidelines, "Use Godot's Theme System",
                "Define colors, fonts, and styles in Theme resources "
                "rather than per-control overrides. This ensures "
                "consistency and simplifies restyling.", ex3);
        }

        if (topic != "scene_organization" && topic != "scripting"
            && topic != "performance" && topic != "ui_design" && topic != "all") {
            return ToolResult::err("BAD_PARAM",
                "Unknown topic: " + topic + ". Use: scene_organization, "
                "scripting, performance, ui_design, or all.");
        }

        Dictionary data;
        data["topic"] = topic;
        data["guidelines"] = guidelines;
        data["count"] = static_cast<int64_t>(guidelines.size());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
