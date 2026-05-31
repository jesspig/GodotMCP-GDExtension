import os
import re


def resolve_path(path: str, project_path: str = "example") -> str:
    if path.startswith("res://"):
        rel = path[6:]
        full = os.path.join(project_path, rel)
        full = full.replace("/", os.sep).replace("\\", os.sep)
        return full
    if not os.path.isabs(path):
        return os.path.join(project_path, path)
    return path


def file_exists(rel_path: str, project_path: str = "example") -> bool:
    full = resolve_path(rel_path, project_path)
    return os.path.isfile(full)


def parse_tscn(path: str) -> dict:
    full_path = resolve_path(path)
    if not os.path.isfile(full_path):
        return {"error": f"file not found: {full_path}", "nodes": {}}

    result = {
        "gd_scene": {},
        "ext_resources": [],
        "sub_resources": [],
        "nodes": {},
        "connections": [],
    }

    with open(full_path, encoding="utf-8") as f:
        lines = f.readlines()

    current_node = None
    for line in lines:
        line_stripped = line.strip()

        if line_stripped.startswith("[gd_scene"):
            attrs = _parse_bracket_attrs(line_stripped)
            result["gd_scene"] = attrs

        elif line_stripped.startswith("[ext_resource"):
            attrs = _parse_bracket_attrs(line_stripped)
            result["ext_resources"].append(attrs)

        elif line_stripped.startswith("[sub_resource"):
            attrs = _parse_bracket_attrs(line_stripped)
            result["sub_resources"].append(attrs)

        elif line_stripped.startswith("[node"):
            attrs = _parse_bracket_attrs(line_stripped)
            name = attrs.get("name", "?")
            current_node = name
            result["nodes"][name] = {"attrs": attrs, "props": {}}

        elif line_stripped.startswith("[connection"):
            conn = _parse_bracket_attrs(line_stripped)
            result["connections"].append(conn)

        elif current_node and "=" in line_stripped and not line_stripped.startswith("["):
            key, _, val = line_stripped.partition("=")
            result["nodes"][current_node]["props"][key.strip()] = val.strip()

    return result


def get_node_property(tscn_path: str, node_name: str, prop: str) -> str | None:
    parsed = parse_tscn(tscn_path)
    node = parsed.get("nodes", {}).get(node_name)
    if not node:
        return None
    return node.get("props", {}).get(prop)


def verify_tscn_has_node(tscn_path: str, node_name: str, expected_type: str = "") -> bool:
    parsed = parse_tscn(tscn_path)
    node = parsed.get("nodes", {}).get(node_name)
    if not node:
        return False
    if expected_type and node.get("attrs", {}).get("type", "") != expected_type:
        return False
    return True


def read_project_godot_value(section: str, key: str, project_path: str = "example") -> str | None:
    full = os.path.join(project_path, "project.godot")
    if not os.path.isfile(full):
        return None
    with open(full, encoding="utf-8") as f:
        lines = f.readlines()
    in_section = False
    for line in lines:
        stripped = line.strip()
        if stripped.startswith(f"[{section}]"):
            in_section = True
            continue
        if in_section and stripped.startswith("["):
            break
        if in_section and "=" in stripped and not stripped.startswith(";"):
            k, _, v = stripped.partition("=")
            if k.strip() == key:
                return v.strip()
    return None


def read_project_godot_raw(project_path: str = "example") -> str:
    full = os.path.join(project_path, "project.godot")
    if os.path.isfile(full):
        with open(full, encoding="utf-8") as f:
            return f.read()
    return ""


def verify_file_contains(file_rel: str, substr: str, project_path: str = "example") -> bool:
    full = resolve_path(file_rel, project_path)
    if not os.path.isfile(full):
        return False
    with open(full, encoding="utf-8", errors="replace") as f:
        content = f.read()
    return substr in content


def _parse_bracket_attrs(text: str) -> dict:
    text = text.strip()
    if "[" in text:
        text = text[text.index("[") + 1:]
    if "]" in text:
        text = text[:text.index("]")]
    parts = text.split(None, 1)
    if len(parts) > 1:
        rest = parts[1]
    else:
        rest = ""
    attrs = {"_type": parts[0] if parts else text}
    for m in re.finditer(r'(\w+)="([^"]*)"', rest):
        attrs[m.group(1)] = m.group(2)
    return attrs
