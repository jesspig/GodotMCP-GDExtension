# Testing Overview

GodotMCP includes a built-in YAML-based test engine that runs inside the Godot editor process. Tests are defined as YAML pipeline files, sent via HTTP POST to the `/run-tests` endpoint.

## Architecture

```
YAML Pipeline File
    |
    v
POST /run-tests (HTTP)
    |
    v
TestEngine::run(yaml)
    |
    v
PipelineParser -> PipelineDef  (parse + validate)
    |
    v
PipelineRunner::run(def)       (execute stages/steps)
    |
    v
Assertions + Disk Verification
    |
    v
JSON Result Report
```

## Components

| Component | Language | Location | Description |
|-----------|----------|----------|-------------|
| TestEngine | C++ | `extensions/src/testing/test_engine.hpp` | Top-level engine |
| PipelineParser | C++ | `extensions/src/testing/pipeline_parser.hpp` | YAML -> PipelineDef |
| PipelineRunner | C++ | `extensions/src/testing/pipeline_runner.hpp` | Pipeline executor |
| PipelineContext | C++ | `extensions/src/testing/pipeline_context.hpp` | Cross-step data store |
| TestOrchestrator | Python | `tests/test_orchestrator.py` | Godot lifecycle + test dispatch |
| YAML tests | YAML | `tests/yaml_tests/` (26 files) | Test definitions |

## How Testing Works

1. A YAML file defines a **pipeline** with stages and steps
2. Each step calls a tool with specific arguments
3. Expected results are defined using the **assertion system**
4. **Disk verification** can check scene files, project settings, and text files
5. The runner executes steps sequentially, supporting dependencies and retries

## Running Tests

### Via Command Line

```bash
uv run python main.py test
```

### Via HTTP (with Godot running)

```bash
curl -X POST http://localhost:9600/run-tests \
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/yaml_tests/00_meta.yaml
```

### Options

| Flag | Description |
|------|-------------|
| `--file PATTERN` | Run specific test files (glob) |
| `--headless` | Only run headless-compatible tests |
| `--keep-open` | Keep Godot open after tests |
| `--no-auto` | Don't auto-start Godot |