#!/usr/bin/env python3
"""
collect_settings.py — 从 Godot 运行时提取项目设置数据

通过 Godot 的 --headless 模式运行 GDScript，从 ProjectSettings 中提取所有
可见设置项（包含特性标签变体），按顶级分类分组生成 YAML 数据库。

用法:
    uv run python tools/collect_settings.py --godot /path/to/godot

输出: extensions/src/built_in/tools/editor_tools/settings/db/*.yaml
"""

import argparse
import os
import subprocess
import sys

try:
    import yaml
except ImportError:
    print("[collect] ERROR: pyyaml not installed. Run: uv add pyyaml", file=sys.stderr)
    sys.exit(1)


GDSCRIPT_TEMPLATE = """extends SceneTree

var _variant_names = {}

func _init():
    _variant_names = {
        0: "NIL", 1: "bool", 2: "int", 3: "float", 4: "String",
        5: "Vector2", 6: "Vector2i", 7: "Rect2", 8: "Rect2i",
        9: "Vector3", 10: "Vector3i", 11: "Transform2D", 12: "Vector4",
        13: "Vector4i", 14: "Plane", 15: "Quaternion", 16: "AABB",
        17: "Basis", 18: "Transform3D", 19: "Projection", 20: "Color",
        21: "StringName", 22: "NodePath", 23: "RID", 24: "Object",
        25: "Callable", 26: "Signal", 27: "Dictionary", 28: "Array",
        29: "PackedByteArray", 30: "PackedInt32Array", 31: "PackedInt64Array",
        32: "PackedFloat32Array", 33: "PackedFloat64Array", 34: "PackedStringArray",
        35: "PackedVector2Array", 36: "PackedVector3Array", 37: "PackedColorArray",
        38: "PackedVector4Array",
    }

    var file = FileAccess.open("res://_settings_export.yaml", FileAccess.WRITE)
    if file == null:
        print("ERROR: cannot open output file")
        quit(1)

    const PROPERTY_USAGE_EDITOR = 4
    const PROPERTY_USAGE_INTERNAL = 128

    var prop_list = ProjectSettings.get_property_list()
    prop_list.sort_custom(_sort_by_name)

    var count = 0
    for prop in prop_list:
        var usage = prop.get("usage", 0)

        # 跳过无 EDITOR 标记的内部设置
        if (usage & PROPERTY_USAGE_EDITOR) == 0:
            continue

        var name = prop["name"]
        var ptype = prop["type"]
        var tn = _variant_names.get(ptype, "unknown")
        var hint = prop.get("hint", 0)
        var hint_string = prop.get("hint_string", "")
        var basic = (usage & 256) != 0
        var restart = (usage & 2048) != 0

        file.store_line("---")
        file.store_line('name: "' + _escape_yaml(name) + '"')
        file.store_line("type: " + str(ptype))
        file.store_line('type_name: "' + tn + '"')
        file.store_line("hint: " + str(hint))
        file.store_line('hint_string: "' + _escape_yaml(hint_string) + '"')
        file.store_line("basic: " + ("true" if basic else "false"))
        file.store_line("restart: " + ("true" if restart else "false"))
        count += 1

    file.close()
    print("Exported " + str(count) + " project settings")
    quit()

func _sort_by_name(a, b):
    return a["name"] < b["name"]

func _escape_yaml(s):
    var result = s.replace("\\\\", "\\\\\\\\")
    result = result.replace("\\"", "\\\\\\"")
    result = result.replace("\\n", "\\\\n")
    result = result.replace("\\r", "\\\\r")
    return result
"""


def main():
    parser = argparse.ArgumentParser(
        description="Extract project settings data from Godot ProjectSettings"
    )
    parser.add_argument(
        "--godot",
        required=True,
        help="Path to Godot 4.6+ executable",
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="Output directory for setting YAML files",
    )
    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))

    out_dir = args.output_dir
    if not out_dir:
        out_dir = os.path.join(script_dir, "..", "extensions", "src",
                                "built_in", "tools", "editor_tools", "settings", "db")

    os.makedirs(out_dir, exist_ok=True)

    example_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "example")
    example_dir = os.path.normpath(example_dir)
    if not os.path.isfile(os.path.join(example_dir, "project.godot")):
        print(f"[collect] ERROR: example/project.godot not found at {example_dir}", file=sys.stderr)
        sys.exit(1)

    script_path = os.path.join(example_dir, "_collect_settings.gd")
    with open(script_path, "w", encoding="utf-8") as f:
        f.write(GDSCRIPT_TEMPLATE)

    try:
        print(f"[collect] Running Godot to extract project settings...")
        result = subprocess.run(
            [args.godot, "--headless", "--path", example_dir, "--script", "_collect_settings.gd"],
            capture_output=True,
            text=True,
            timeout=120,
        )

        if result.returncode != 0:
            print(f"[collect] ERROR: Godot exited with code {result.returncode}", file=sys.stderr)
            print(result.stderr, file=sys.stderr)
            sys.exit(1)

        export_path = os.path.join(example_dir, "_settings_export.yaml")
        if not os.path.exists(export_path):
            print(f"[collect] ERROR: Export file not found at {export_path}", file=sys.stderr)
            sys.exit(1)

        with open(export_path, "r", encoding="utf-8") as f:
            documents = list(yaml.safe_load_all(f))

        def load_existing(dir_path):
            existing = {}
            if os.path.isdir(dir_path):
                for fname in os.listdir(dir_path):
                    if not fname.endswith(".yaml"):
                        continue
                    cat = fname[:-5]
                    fpath = os.path.join(dir_path, fname)
                    with open(fpath, encoding="utf-8") as f:
                        data = yaml.safe_load(f)
                    if data:
                        existing[cat] = data
            return existing

        existing_db = load_existing(out_dir)

        settings_by_category: dict[str, list[dict]] = {}
        for doc in documents:
            if not doc or "name" not in doc:
                continue
            name: str = doc["name"]
            top_cat = name.split("/")[0]
            if top_cat not in settings_by_category:
                settings_by_category[top_cat] = []

            entry = {
                "name": name,
                "type": doc["type"],
                "type_name": doc.get("type_name", "unknown"),
                "hint": doc.get("hint", 0),
                "hint_string": doc.get("hint_string", ""),
                "basic": doc.get("basic", False),
                "restart": doc.get("restart", False),
            }
            settings_by_category[top_cat].append(entry)

        cat_count = 0
        for cat_name in sorted(settings_by_category.keys()):
            settings = settings_by_category[cat_name]
            settings.sort(key=lambda s: s["name"])

            merged = {
                "category": cat_name,
                "settings": settings,
            }

            yaml_path = os.path.join(out_dir, f"{cat_name}.yaml")
            with open(yaml_path, "w", encoding="utf-8") as f:
                yaml.dump(merged, f, default_flow_style=False, allow_unicode=True, sort_keys=False)
            cat_count += 1

        total = sum(len(v) for v in settings_by_category.values())
        print(f"[collect] Generated {cat_count} category YAML files in {out_dir} ({total} settings total)")

    finally:
        if os.path.exists(script_path):
            os.remove(script_path)
        # Keep export file for debugging; remove on success only
        if 'result' in locals() and result.returncode == 0:
            export_yaml = os.path.join(example_dir, "_settings_export.yaml")
            if os.path.exists(export_yaml):
                os.remove(export_yaml)


if __name__ == "__main__":
    main()