#include "prompt_provider.hpp"

namespace godot_mcp {
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

} // namespace

Dictionary PromptProvider::make_prompt_definition(const String &name, const String &description, const Array &arguments) {
    Dictionary d;
    d["name"] = name;
    d["description"] = description;
    if (arguments.size() > 0) {
        d["arguments"] = arguments;
    }
    return d;
}

Array PromptProvider::list_prompts() const {
    Array prompts;

    // create_scene
    {
        Array args;
        Dictionary arg;
        arg["name"] = "scene_type";
        arg["description"] = "Scene type: 2d, 3d, or empty";
        arg["required"] = true;
        args.append(arg);
        prompts.append(make_prompt_definition("create_scene",
            "Guide for creating a new scene with recommended setup", args));
    }

    // create_node
    {
        Array args;
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
        prompts.append(make_prompt_definition("create_node",
            "Guide for adding a node to the scene", args));
    }

    // fix_error
    {
        Array args;
        Dictionary arg;
        arg["name"] = "error_text";
        arg["description"] = "The error message to analyze";
        arg["required"] = true;
        args.append(arg);
        prompts.append(make_prompt_definition("fix_error",
            "Analyze an editor error or script error and suggest fixes", args));
    }

    // explain_node
    {
        Array args;
        Dictionary arg;
        arg["name"] = "node_type";
        arg["description"] = "The node class name to explain";
        arg["required"] = true;
        args.append(arg);
        prompts.append(make_prompt_definition("explain_node",
            "Explain what a Godot node type does and common usage patterns", args));
    }

    // code_review
    {
        Array args;
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
        prompts.append(make_prompt_definition("code_review",
            "Review GDScript or C# script for best practices and potential issues", args));
    }

    return prompts;
}

Dictionary PromptProvider::get_prompt(const String &name, const Dictionary &args) const {
    if (name == "create_scene") {
        return make_create_scene_prompt(args);
    }
    if (name == "create_node") {
        return make_create_node_prompt(args);
    }
    if (name == "fix_error") {
        return make_fix_error_prompt(args);
    }
    if (name == "explain_node") {
        return make_explain_node_prompt(args);
    }
    if (name == "code_review") {
        return make_code_review_prompt(args);
    }
    return Dictionary();
}

Dictionary PromptProvider::make_create_scene_prompt(const Dictionary &args) {
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

Dictionary PromptProvider::make_create_node_prompt(const Dictionary &args) {
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

Dictionary PromptProvider::make_fix_error_prompt(const Dictionary &args) {
    String error_text = args.get("error_text", "");
    String prompt_text = "Error analysis for: " + error_text + "\n\n"
                         "Suggested steps:\n"
                         "1. Check for typos in node paths and variable names.\n"
                         "2. Verify that all required nodes are present in the scene.\n"
                         "3. Check signal connections for correct target methods.\n"
                         "4. Ensure exported variables are assigned in the editor.\n";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Error fix guidance";
    result["messages"] = messages;
    return result;
}

Dictionary PromptProvider::make_explain_node_prompt(const Dictionary &args) {
    String node_type = args.get("node_type", "Node");
    String prompt_text = "The " + node_type + " node in Godot 4.6:\n"
                         "- Inherits from: depends on the type\n"
                         "- Primary use: [description depends on node type]\n"
                         "- Key properties: [list common properties]\n"
                         "- Common child nodes: [common children]\n"
                         "- Best practices: [usage tips]\n";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Explanation of " + node_type;
    result["messages"] = messages;
    return result;
}

Dictionary PromptProvider::make_code_review_prompt(const Dictionary &args) {
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

} // namespace godot_mcp
