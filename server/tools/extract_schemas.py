#!/usr/bin/env python3
"""Extract tool schemas from Rust tool_registry.rs and generate registry.py.

Usage:
    python tools/extract_schemas.py              # print to stdout
    python tools/extract_schemas.py --write       # write in place
"""

import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
RUST_FILE = REPO_ROOT / "crates" / "server" / "src" / "tool_registry.rs"
REGISTRY_PY = REPO_ROOT / "server" / "src" / "godot_mcp_server" / "registry.py"


def extract_tools(source: str) -> list[dict]:
    """Extract all (name, description, schema) from the Rust defaults vector by
    looking for the pattern ("tool_name", "description", schema_or_empty),."""

    start = source.find("let defaults: Vec<(&str, &str, serde_json::Value)> = vec![")
    start = source.index("[", start) + 1
    end = source.find("];", start)
    body = source[start:end]

    # Strategy: pre-process to replace raw strings r#"..."# with placeholders
    # so we can find entry boundaries without getting confused by internal content.
    #
    # Replace r#"..."# with a placeholder, and r##"..."## similarly.
    # Track the mapping to restore later.

    raw_map = {}
    counter = [0]

    def replace_raw(m):
        content = m.group(1)
        key = f"__RAW_{counter[0]}__"
        counter[0] += 1
        raw_map[key] = content
        return key

    # Replace r##"..."## (double hash)
    body = re.sub('r\x23\x23\x22((?:[^\x22]|\x22(?!\x23\x23))*)\x22\x23\x23', replace_raw, body)
    # Replace r#"..."# (single hash)
    body = re.sub('r\x23\x22((?:[^\x22]|\x22(?!\x23))*)\x22\x23', replace_raw, body)

    # Now extract entries — each entry is (...), at the top level
    entries = []
    i = 0
    paren_depth = 0
    entry_start = None
    in_str = False

    while i < len(body):
        ch = body[i]
        next2 = body[i:i+2]

        # Skip line comments
        if not in_str and next2 == "//":
            nl = body.find("\n", i)
            i = nl if nl != -1 else len(body)
            continue
        # Skip block comments
        if not in_str and next2 == "/*":
            end_c = body.find("*/", i+2)
            i = end_c + 2 if end_c != -1 else len(body)
            continue

        if not in_str and ch == '"':
            in_str = True
        elif in_str and ch == '"' and (i == 0 or body[i-1] != "\\"):
            in_str = False

        if not in_str:
            if ch == "(":
                if paren_depth == 0:
                    entry_start = i
                paren_depth += 1
            elif ch == ")":
                paren_depth -= 1
                if paren_depth == 0 and entry_start is not None:
                    entry_text = body[entry_start:i+1]
                    parsed = _parse_entry(entry_text, raw_map)
                    if parsed:
                        entries.append(parsed)
                    entry_start = None

        i += 1

    return entries


def _parse_entry(text: str, raw_map: dict) -> dict | None:
    """Parse a single (name, desc, schema) tuple."""
    text = text.strip().strip("()").strip()

    # Extract name via regex (1st quoted string)
    m_name = re.match(r'\s*"((?:[^\\"]|\\.)*)"', text)
    if not m_name:
        return None
    name = m_name.group(1)
    rest = text[m_name.end():]

    # Extract description (2nd quoted string)
    m_desc = re.match(r'\s*,\s*"((?:[^\\"]|\\.)*)"', rest)
    if not m_desc:
        return None
    desc = m_desc.group(1)
    rest = rest[m_desc.end():]

    # The rest is schema — strip leading comma and any extra
    schema_src = rest.strip().lstrip(",").strip()

    # Check for empty (hardcoded empty schema)
    if schema_src.startswith("empty"):
        schema = {"type": "object", "properties": {}, "required": []}
    elif schema_src.startswith("schema("):
        # Extract content inside schema(...)
        # schema(r#"..."#, &[...]) → extract the JSON part
        # But raw strings are replaced, so we have schema(__RAW_N__, &[...])
        m = re.match(r'schema\(\s*([A-Z_][A-Z_0-9]*)\s*,\s*&\[(.*?)\]\s*\)', schema_src, re.DOTALL)
        if m:
            raw_key = m.group(1).strip()
            props_raw = raw_map.get(raw_key, "{}")
            req_raw = m.group(2).strip()
            try:
                properties = json.loads(props_raw) if props_raw.strip() else {}
            except json.JSONDecodeError:
                properties = {}
            required = [
                r.strip().strip('"')
                for r in re.split(r",\s*", req_raw)
                if r.strip()
            ] if req_raw else []
            schema = {"type": "object", "properties": properties, "required": required}
        else:
            schema = {"type": "object", "properties": {}, "required": []}
    else:
        schema = {"type": "object", "properties": {}, "required": []}

    return {"name": name, "description": desc, "input_schema": schema}


def generate_py(tools: list[dict]) -> str:
    """Generate registry.py content."""
    lines = [
        "# Auto-generated from crates/server/src/tool_registry.rs",
        "# Run: python tools/extract_schemas.py --write",
        "# flake8: noqa",
        "",
        "from typing import Any",
        "",
        "from godot_mcp_server.protocol import ToolInfo, ToolListUpdate, ToolState",
        "",
        "",
        "class ToolRegistry:",
        '    """Registry of all tool schemas, matching Rust tool_registry.rs."""',
        "",
        "    _TOOLS: list[tuple[str, str, dict[str, Any]]] = [",
    ]

    for i, t in enumerate(tools):
        comma = ","
        name_j = json.dumps(t["name"], ensure_ascii=False)
        desc_j = json.dumps(t["description"], ensure_ascii=False)
        schema_s = json.dumps(t["input_schema"], ensure_ascii=False, sort_keys=False)
        lines.append(f"        ({name_j}, {desc_j}, {schema_s}){comma}")

    lines += [
        "    ]",
        "",
        "    def __init__(self) -> None:",
        "        self._tools: dict[str, ToolInfo] = {}",
        "        for name, desc, schema in self._TOOLS:",
        "            self._tools[name] = ToolInfo(",
        "                name=name,",
        "                description=desc,",
        "                input_schema=schema,",
        "                enabled=True,",
        "            )",
        "",
        "    @property",
        "    def total(self) -> int:",
        "        return len(self._tools)",
        "",
        "    def get_all_tools(self) -> list[ToolInfo]:",
        "        return list(self._tools.values())",
        "",
        "    def get_enabled_tools(self) -> list[ToolInfo]:",
        "        return [t for t in self._tools.values() if t.enabled]",
        "",
        "    def has_tool(self, name: str) -> bool:",
        "        return name in self._tools",
        "",
        "    def is_tool_enabled(self, name: str) -> bool:",
        "        t = self._tools.get(name)",
        "        return t.enabled if t is not None else False",
        "",
        "    def set_tool_enabled(self, name: str, enabled: bool) -> bool:",
        "        if name not in self._tools:",
        "            return False",
        "        self._tools[name].enabled = enabled",
        "        return True",
        "",
        "    def register_tool(self, name: str, description: str, schema: dict) -> None:",
        "        self._tools[name] = ToolInfo(",
        "            name=name,",
        "            description=description,",
        "            input_schema=schema,",
        "            enabled=True,",
        "        )",
        "",
        "    def update_from_notification(self, update: ToolListUpdate) -> None:",
        "        for ts in update.tools:",
        "            if ts.name in self._tools:",
        "                self._tools[ts.name].enabled = ts.enabled",
        "",
        "    def tool_count(self) -> tuple[int, int]:",
        "        enabled = sum(1 for t in self._tools.values() if t.enabled)",
        "        return (enabled, len(self._tools))",
        "",
        "",
    ]

    return "\n".join(lines)


def main():
    source = RUST_FILE.read_text(encoding="utf-8")
    tools = extract_tools(source)

    names = {t["name"] for t in tools}
    print(f"Extracted {len(tools)} tools ({len(names)} unique)", file=sys.stderr)
    if len(tools) != 125:
        print(f"WARNING: Expected 125, got {len(tools)}", file=sys.stderr)

    py_code = generate_py(tools)

    if "--write" in sys.argv:
        REGISTRY_PY.parent.mkdir(parents=True, exist_ok=True)
        REGISTRY_PY.write_text(py_code, encoding="utf-8")
        print(f"Written to {REGISTRY_PY}", file=sys.stderr)
    else:
        print(py_code)


if __name__ == "__main__":
    main()
