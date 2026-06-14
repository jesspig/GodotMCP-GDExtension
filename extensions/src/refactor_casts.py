"""Replace C-style casts with static_cast in extensions/src.

Approach: recursively find and replace casts from innermost to outermost.
Expression bounds are determined by tracking paren/bracket balance.
"""
import os
import re
import sys

SRC = os.path.dirname(os.path.abspath(__file__))

TYPES = [
    'unsigned int', 'unsigned',  # must be before shorter matches
    'real_t', 'float', 'double', 'int', 'bool',
    'int64_t', 'int32_t', 'uint32_t', 'uint16_t', 'size_t',
    'String', 'Array', 'Dictionary',
    'Vector2', 'Vector2i', 'Vector3', 'Vector4', 'Color',
    'Error', 'Variant::Type',
    'AudioStreamPlayer3D', 'NavigationPolygon', 'Resource', 'NodePath',
    'Key',
]

# Order by length descending so longer types match first
TYPE_RE = '(' + '|'.join(re.escape(t) for t in sorted(TYPES, key=len, reverse=True)) + ')'
CAST_RE = re.compile(r'\(' + TYPE_RE + r'\)')

CONTINUATION_CHARS = set('_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[(.')


def collect_files():
    files = []
    for root, dirs, fnames in os.walk(SRC):
        skip = False
        for part in root.split(os.sep):
            if part in ('_deps', 'build'):
                skip = True
                break
        if skip:
            continue
        for f in fnames:
            if f.endswith('.hpp') or f.endswith('.cpp'):
                files.append(os.path.join(root, f))
    return sorted(files)


def expr_end(s, start):
    """Find where the expression being cast ends.
    Returns exclusive end index.
    Expression ends at `,`, `;`, unmatched `)`, or top-level binary operator."""
    i = start
    paren = 0
    brack = 0
    angle = 0  # track <> for templates
    n = len(s)
    started = False

    while i < n:
        ch = s[i]

        # Skip leading whitespace before expression starts
        if not started:
            if ch in ' \t':
                i += 1
                continue
            # Expression must start with a valid token
            if ch.isalpha() or ch == '_' or ch == '(' or ch == '"' or ch in '\'+-*&!~':
                started = True
                i += 1
                continue
            # Not a valid start
            break

        # --- Expression has started ---

        if ch == '(':
            paren += 1
        elif ch == ')':
            if paren == 0:
                return i  # unmatched - ends expression
            paren -= 1
            if paren == 0:
                j = i + 1
                while j < n and s[j] in ' \t':
                    j += 1
                if j < n and s[j] in CONTINUATION_CHARS:
                    i += 1
                    continue
                return i + 1
        elif ch == '[':
            brack += 1
        elif ch == ']':
            if brack == 0:
                return i
            brack -= 1
        elif ch == '<':
            if paren == 0 and brack == 0:
                # Could be template or less-than: treat as operator (stop)
                return i
            angle += 1
        elif ch == '>':
            if angle > 0:
                angle -= 1
            elif paren == 0 and brack == 0:
                return i  # Greater-than operator
        elif ch == '-' and paren == 0 and brack == 0:
            if i + 1 < n and s[i + 1] == '>':
                # -> member access, part of expression
                i += 2
                continue
            return i  # binary minus (or unary after start)
        elif ch == '+' and paren == 0 and brack == 0:
            return i  # binary plus (or unary after start)
        elif ch == '*' and paren == 0 and brack == 0:
            return i  # multiplication (or deref after start)
        elif ch == '/' and paren == 0 and brack == 0:
            return i  # division
        elif ch == '%' and paren == 0 and brack == 0:
            return i
        elif ch == '=' and paren == 0 and brack == 0:
            return i  # assignment or ==
        elif ch == '!' and paren == 0 and brack == 0:
            if i + 1 < n and s[i + 1] == '=':
                return i  # != operator
            # Unary ! continues expression
            i += 1
            continue
        elif ch == '&' and paren == 0 and brack == 0:
            return i  # bitwise AND (address-of only valid at start)
        elif ch == '|' and paren == 0 and brack == 0:
            return i  # bitwise OR
        elif ch == '^' and paren == 0 and brack == 0:
            return i
        elif ch == '?' and paren == 0 and brack == 0:
            return i  # ternary
        elif ch == ':' and paren == 0 and brack == 0:
            if i + 1 < n and s[i + 1] == ':':
                i += 1  # scope resolution
                continue
            return i  # ternary else or label
        elif ch in ',;' and paren == 0 and brack == 0:
            return i

        i += 1

    return n


def replace_line(line):
    """Replace all C-style casts in a single line."""
    # Save (void) casts (unused variable suppression)
    void_map = {}
    def _save(m):
        idx = len(void_map)
        void_map[idx] = m.group(0)
        return f'\0VOID{idx}\0'
    line = re.sub(r'\(void\)\s*[a-zA-Z_][a-zA-Z0-9_]*', _save, line)

    # Iteratively replace casts from left to right
    changed = True
    while changed:
        changed = False
        m = CAST_RE.search(line)
        if m:
            ctype = m.group(1)
            pos = m.end()
            end = expr_end(line, pos)
            if end > pos:
                expr = line[pos:end]
                if expr and (expr[0].isalpha() or expr[0] == '_' or expr[0] == '(' or expr[0] == '"' or expr[0] == '-' or expr[0] == '+' or expr[0] == '&' or expr[0] == '*' or expr[0] == '!'):
                    line = line[:m.start()] + f'static_cast<{ctype}>({expr})' + line[end:]
                    changed = True

    # Restore (void) casts
    for idx, val in void_map.items():
        line = line.replace(f'\0VOID{idx}\0', val)

    return line


def process_file(fpath):
    with open(fpath, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    new_lines = []
    changed = False
    for line in lines:
        if CAST_RE.search(line):
            nl = replace_line(line)
            if nl != line:
                changed = True
                new_lines.append(nl)
                continue
        new_lines.append(line)

    if changed:
        with open(fpath, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        return True
    return False


def main():
    files = collect_files()
    print(f"Total files: {len(files)}", flush=True)

    modified = []
    for fpath in files:
        try:
            if process_file(fpath):
                rel = os.path.relpath(fpath, SRC)
                modified.append(rel)
                print(f"  {rel}", flush=True)
        except Exception as e:
            print(f"  ERROR {os.path.relpath(fpath, SRC)}: {e}", flush=True)

    print(f"\nModified {len(modified)} files", flush=True)
    for m in sorted(modified):
        print(f"  {m}", flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
