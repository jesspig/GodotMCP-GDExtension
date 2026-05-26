"""Patch Cython-generated entry.c to embed PYTHONHOME.

Inserts PyConfig_SetString(&config, &config.home, L"...") before
Py_InitializeFromConfig so the compiled exe finds Python stdlib
without needing PYTHONHOME env var.
"""

import sys


def patch_entry_c(input_path: str, output_path: str, python_home: str) -> None:
    with open(input_path, "r", encoding="utf-8") as f:
        content = f.read()

    wpath = python_home.replace("\\", "\\\\")
    insertion = (
        f'    status = PyConfig_SetString(&config, &config.home, L"{wpath}");\n'
        f'    if (PyStatus_Exception(status)) {{\n'
        f'        PyConfig_Clear(&config);\n'
        f'        return 1;\n'
        f'    }}\n'
    )

    marker = "config.parse_argv = 0;"
    if marker not in content:
        print(f"ERROR: marker '{marker}' not found in {input_path}", file=sys.stderr)
        sys.exit(1)

    content = content.replace(marker, marker + "\n" + insertion, 1)

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"Patched {output_path}: home = L\"{python_home}\"")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <input.c> <output.c> <python_home_path>", file=sys.stderr)
        sys.exit(1)
    patch_entry_c(sys.argv[1], sys.argv[2], sys.argv[3])
