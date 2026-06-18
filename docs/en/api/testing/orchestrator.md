# Test Orchestrator

The Python test orchestrator manages the Godot editor process and dispatches YAML test files.

## Command

```bash
uv run python main.py test
```

## Options

| Flag | Description |
|------|-------------|
| `--file PATTERN` | Run specific test files (glob pattern on YAML names) |
| `--headless` | Only run tests marked `headless: true` |
| `--keep-open` | Keep Godot running after tests complete |
| `--no-auto` | Don't auto-start Godot (connect to already-running instance) |

## Architecture

```
test_orchestrator.py
    |
    +-- GodotManager (godot_manager.py)
    |     Manages Godot process lifecycle
    |     - Start with --editor (optionally --headless)
    |     - Ping MCP endpoint until ready
    |     - Kill on completion
    |
    +-- YAML Discovery
    |     - Scan tests/yaml_tests/
    |     - Filter by headless compatibility
    |     - Filter by --file pattern
    |
    +-- Test Dispatch
    |     - POST each YAML to /run-tests
    |     - 120s timeout per file
    |     - Retry headless tests without --headless on failure
    |
    +-- Report (report.py)
          - Generate JSON + Markdown reports
          - Per-phase pass/fail/skip statistics
          - Per-tool aggregation
```

## Output

Results are written to `tests/output/`:

```
tests/output/
  report.json           # Machine-readable report
  report.md             # Human-readable report
  <timestamp>_*.json    # Individual per-run reports
```

## Report Structure

```json
{
  "phases": {
    "00_meta": {
      "pass": 7,
      "fail": 0,
      "skip": 0
    }
  },
  "summary": {
    "phases": 26,
    "pass": 150,
    "fail": 0,
    "skip": 3,
    "call_count": 300,
    "call_success": 298,
    "unique_tools": 100,
    "unique_success": 99,
    "duration_ms": 45000
  },
  "tool_stats": {
    "add_node": { "total": 10, "pass": 10, "fail": 0, "failing_phases": [] },
    "delete_node": { "total": 5, "pass": 4, "fail": 1, "failing_phases": ["04_scene_tree"] }
  }
}
```

## Adding Test Files

1. Create a new YAML file in `tests/yaml_tests/`
2. Follow the [Pipeline Format](/api/testing/pipeline-format)
3. Run: `uv run python main.py test --file my_new_test.yaml`