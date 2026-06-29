#include "prompt_provider.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace godot_mcp {
namespace prompt_provider {
using namespace godot;

namespace {

Dictionary make_text_message(const String &text) {
    Dictionary msg;
    msg["role"] = "user";
    Dictionary content;
    content["type"] = "text";
    content["text"] = text;
    msg["content"] = content;
    return msg;
}

// ---- Prompt factories ----

Dictionary make_create_scene_prompt(const Dictionary &args) {
    String scene_type = args.get("scene_type", "2d");
    String prompt_text;
    if (scene_type == "2d") {
        prompt_text = "Create a new 2D scene using the 'New Scene' button, then add a Node2D as root. "
                      "For the root, set a meaningful name like 'Game' or 'Level'. "
                      "Then add child nodes as needed (Sprite2D, CollisionShape2D, etc.). "
                      "Save the scene using Ctrl+S.";
    } else if (scene_type == "3d") {
        prompt_text = "Create a new 3D scene, add a Node3D as root. "
                      "Consider adding a WorldEnvironment for lighting, a Camera3D for view, and MeshInstance3D for objects.";
    } else {
        prompt_text = "Create a new empty scene with any root node type you need.";
    }

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Guide for creating a " + scene_type + " scene";
    result["messages"] = messages;
    return result;
}

Dictionary make_create_node_prompt(const Dictionary &args) {
    String node_type = args.get("node_type", "Node");
    String prompt_text = "To add a " + node_type + " to the scene: "
                         "1. Ensure the scene is open and the target parent is selected. "
                         "2. Use the add_node tool with parent_path and class_name='" + node_type + "'. "
                         "3. Configure the node's properties as needed.";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Guide for adding a " + node_type;
    result["messages"] = messages;
    return result;
}

Dictionary make_fix_error_prompt(const Dictionary &args) {
    String error_text = args.get("error_text", "");

    // Try to get live error count from editor debugger
    int editor_errors = -1;
    EditorInterface *ei = EditorInterface::get_singleton();
    if (ei) {
        auto *base = ei->get_base_control();
        if (base) {
            Array nodes = base->find_children("*", "EditorDebuggerNode", true, false);
            if (nodes.size() > 0) {
                auto *dbg = Object::cast_to<Node>(nodes[0]);
                if (dbg) {
                    Object *active = dbg->call("get_current_debugger");
                    if (active) {
                        editor_errors = static_cast<int>(static_cast<int64_t>(active->call("get_error_count")));
                    }
                }
            }
        }
    }

    String error_context;
    if (editor_errors >= 0) {
        error_context = String("\nEditor error count: ") + String::num_int64(editor_errors);
    }

    String prompt_text = "Error analysis for: " + error_text + error_context + "\n\n"
                         "Suggested steps:\n"
                         "1. Check for typos in node paths and variable names.\n"
                         "2. Verify that all required nodes are present in the scene.\n"
                         "3. Check signal connections for correct target methods.\n"
                         "4. Ensure exported variables are assigned in the editor.\n";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Error fix guidance (live context)";
    result["messages"] = messages;
    return result;
}

Dictionary make_explain_node_prompt(const Dictionary &args) {
    String node_type = args.get("node_type", "Node");

    // Build inheritance chain from ClassDB
    String inherits_info;
    if (ClassDB::class_exists(node_type)) {
        String parent = ClassDB::get_parent_class(node_type);
        Array chain;
        chain.append(node_type);
        String cur = node_type;
        while (!parent.is_empty()) {
            chain.append(parent);
            cur = parent;
            parent = ClassDB::get_parent_class(cur);
        }
        inherits_info = "Inheritance: ";
        for (int i = 0; i < chain.size(); i++) {
            if (i > 0) inherits_info += " → ";
            inherits_info += String(chain[i]);
        }
        int method_count = ClassDB::class_get_method_list(node_type, false).size();
        int prop_count = ClassDB::class_get_property_list(node_type, false).size();
        int sig_count = ClassDB::class_get_signal_list(node_type, false).size();
        inherits_info += "\n- Methods: " + String::num_int64(method_count);
        inherits_info += ", Properties: " + String::num_int64(prop_count);
        inherits_info += ", Signals: " + String::num_int64(sig_count);
    } else {
        inherits_info = "Class not found in ClassDB. Inheritance information unavailable.";
    }

    String prompt_text = "The " + node_type + " node in Godot 4.6:\n"
                         "- " + inherits_info + "\n"
                         "- Primary use: [description depends on node type]\n"
                         "- Key properties: [list common properties]\n"
                         "- Common child nodes: [common children]\n"
                         "- Best practices: [usage tips]\n";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Explanation of " + node_type + " with ClassDB data";
    result["messages"] = messages;
    return result;
}

Dictionary make_debug_session_prompt(const Dictionary &args) {
    String prompt_text = "To start a debug session in Godot:\n"
                         "1. Set breakpoints in your script using the set_breakpoints tool or by clicking in the script editor gutter.\n"
                         "2. Start the project with 'Run Project' (F5) or use start_project tool.\n"
                         "3. When a breakpoint is hit, the editor will pause. Use step_over, step_into, or step_out to navigate.\n"
                         "4. Inspect variables via the debugger panel or get_game_node_property tool.\n"
                         "5. Continue execution (F12) or stop the debug session (stop_project).\n"
                         "Useful tools: start_project, stop_project, get_game_node_property, call_method_in_game.";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Debug session setup and workflow guide";
    result["messages"] = messages;
    return result;
}

Dictionary make_animate_node_prompt(const Dictionary &args) {
    String prompt_text = "To animate a node in Godot:\n"
                         "1. Select the node you want to animate.\n"
                         "2. Create an AnimationPlayer node as a child (or sibling).\n"
                         "3. Select the AnimationPlayer and open the Animation panel (bottom panel).\n"
                         "4. Create a new animation track via 'Animation' → 'New'.\n"
                         "5. Select the target node, click the key icon next to a property (e.g., position, rotation, scale).\n"
                         "6. Move to a different time on the timeline and change the property to insert another keyframe.\n"
                         "7. Use looping or single-shot mode as needed.\n"
                         "Useful tools: add_node, set_node_property, get_node_property, play_animation.";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Guide for creating animations with AnimationPlayer";
    result["messages"] = messages;
    return result;
}

Dictionary make_shadow_edit_prompt(const Dictionary &args) {
    String prompt_text = "To use the shadow scene (non-destructive editing) workflow:\n"
                         "1. Capture the current scene state using stage_scene_change.\n"
                         "2. Make changes using scene tools (add_node, set_node_property, etc.).\n"
                         "3. Preview the accumulated changes with preview_change.\n"
                         "4. Apply all changes atomically with apply_changes (single undo step).\n"
                         "5. Or discard all staged changes with discard_changes.\n"
                         "Useful tools: stage_scene_change, preview_change, apply_changes, discard_changes.";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Shadow scene and non-destructive editing workflow";
    result["messages"] = messages;
    return result;
}

Dictionary make_export_project_prompt(const Dictionary &args) {
    String target = args.get("target", "default");
    String prompt_text = "To export a project for " + target + ":\n"
                         "1. Open the Export dialog: Project → Export.\n"
                         "2. Add a new preset for your target platform (" + target + ").\n"
                         "3. Configure export options: output path, architecture, textures, etc.\n"
                         "4. Click 'Export Project' or use the command-line exporter.\n"
                         "5. For repeated exports, save presets in export_presets.cfg.\n"
                         "Useful tools: get_project_settings, export_project.";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Export project for " + target + " platform";
    result["messages"] = messages;
    return result;
}

Dictionary make_code_review_prompt(const Dictionary &args) {
    String script_path = args.get("script_path", "");
    String language = args.get("language", "gdscript");
    String prompt_text = "Review the script at " + script_path + ":\n"
                         "- Check for proper indentation and formatting.\n"
                         "- Verify type hints and annotations.\n"
                         "- Look for potential null reference errors.\n"
                         "- Check for proper node path references.\n"
                         "- Verify signal connection patterns.\n"
                         "- Suggest performance improvements.\n";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Code review for " + script_path;
    result["messages"] = messages;
    return result;
}

// ---- Data table ----

struct PromptEntry {
    const char *name;
    const char *description;
    Dictionary (*factory)(const Dictionary &);
    void (*add_args)(Array &);
};

void add_scene_type_arg(Array &args) {
    Dictionary arg;
    arg["name"] = "scene_type";
    arg["description"] = "Scene type: 2d, 3d, or empty";
    arg["required"] = true;
    args.append(arg);
}

void add_create_node_args(Array &args) {
    Dictionary arg1;
    arg1["name"] = "parent_path";
    arg1["description"] = "Path to the parent node";
    arg1["required"] = true;
    args.append(arg1);
    Dictionary arg2;
    arg2["name"] = "node_type";
    arg2["description"] = "Type of node to create (e.g., Node2D, Control, Sprite2D)";
    arg2["required"] = true;
    args.append(arg2);
}

void add_error_text_arg(Array &args) {
    Dictionary arg;
    arg["name"] = "error_text";
    arg["description"] = "The error message to analyze";
    arg["required"] = true;
    args.append(arg);
}

void add_node_type_arg(Array &args) {
    Dictionary arg;
    arg["name"] = "node_type";
    arg["description"] = "The node class name to explain";
    arg["required"] = true;
    args.append(arg);
}

void add_code_review_args(Array &args) {
    Dictionary arg1;
    arg1["name"] = "script_path";
    arg1["description"] = "Path to the script file to review";
    arg1["required"] = true;
    args.append(arg1);
    Dictionary arg2;
    arg2["name"] = "language";
    arg2["description"] = "Language: gdscript or csharp";
    arg2["required"] = false;
    args.append(arg2);
}

void add_no_args(Array &) {}

void add_export_target_arg(Array &args) {
    Dictionary arg;
    arg["name"] = "target";
    arg["description"] = "Export target platform (e.g., Windows, Linux, macOS, Android, Web)";
    arg["required"] = false;
    args.append(arg);
}

const PromptEntry kPrompts[] = {
    { "create_scene",   "Guide for creating a new scene with recommended setup",   &make_create_scene_prompt,    &add_scene_type_arg },
    { "create_node",    "Guide for adding a node to the scene",                     &make_create_node_prompt,     &add_create_node_args },
    { "fix_error",      "Analyze an editor error or script error and suggest fixes with live context", &make_fix_error_prompt,    &add_error_text_arg },
    { "explain_node",   "Explain what a Godot node type does and common usage patterns with ClassDB data", &make_explain_node_prompt, &add_node_type_arg },
    { "code_review",    "Review GDScript or C# script for best practices and potential issues", &make_code_review_prompt,   &add_code_review_args },
    { "debug_session",  "Guide for setting up and running a debug session with breakpoints", &make_debug_session_prompt, &add_no_args },
    { "animate_node",   "Guide for creating animations using AnimationPlayer",      &make_animate_node_prompt,    &add_no_args },
    { "shadow_edit",    "Guide for non-destructive editing with shadow scenes",     &make_shadow_edit_prompt,     &add_no_args },
    { "export_project", "Guide for exporting the project to a target platform",     &make_export_project_prompt,  &add_export_target_arg },
};

constexpr int kPromptCount = sizeof(kPrompts) / sizeof(kPrompts[0]);

} // namespace

Array list_prompts() {
    Array prompts;
    for (int i = 0; i < kPromptCount; i++) {
        Dictionary d;
        d["name"] = String(kPrompts[i].name);
        d["description"] = String(kPrompts[i].description);
        Array args;
        kPrompts[i].add_args(args);
        if (args.size() > 0) {
            d["arguments"] = args;
        }
        prompts.append(d);
    }
    return prompts;
}

Dictionary get_prompt(const String &name, const Dictionary &args) {
    for (int i = 0; i < kPromptCount; i++) {
        if (name == String(kPrompts[i].name)) {
            return kPrompts[i].factory(args);
        }
    }
    return Dictionary();
}

} // namespace prompt_provider
} // namespace godot_mcp
