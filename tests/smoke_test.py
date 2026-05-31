"""Quick smoke test to verify all imports and basic structure."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))

from test_context import TestContext
from test_phases.base import ToolTest, TestResult, PhaseRunner, PhaseReport
from report import TestReport
from godot_manager import GodotManager
from mcp_client import create_mcp_client, list_tools
from file_verifier import (
    parse_tscn, verify_tscn_has_node, file_exists,
    verify_project_godot, read_project_godot_raw,
)
from test_phases import (
    phase_01_connect, phase_02_net_check, phase_03_read_only,
    phase_04_scene_node, phase_05_property_2d, phase_06_property_3d,
    phase_07_collision, phase_08_find, phase_09_script,
    phase_10_scene_mgmt, phase_11_search, phase_12_input,
    phase_13_project_write, phase_14_editor_control, phase_15_plugin,
    phase_16_undo, phase_17_csharp, phase_18_cleanup,
)

phases = [
    ('connect', phase_01_connect.TOOL_TESTS),
    ('net_check', phase_02_net_check.TOOL_TESTS),
    ('read_only', phase_03_read_only.TOOL_TESTS),
    ('scene_node', phase_04_scene_node.TOOL_TESTS),
    ('property_2d', phase_05_property_2d.TOOL_TESTS),
    ('property_3d', phase_06_property_3d.TOOL_TESTS),
    ('collision', phase_07_collision.TOOL_TESTS),
    ('find', phase_08_find.TOOL_TESTS),
    ('script', phase_09_script.TOOL_TESTS),
    ('scene_mgmt', phase_10_scene_mgmt.TOOL_TESTS),
    ('search', phase_11_search.TOOL_TESTS),
    ('input', phase_12_input.TOOL_TESTS),
    ('project_write', phase_13_project_write.TOOL_TESTS),
    ('editor_control', phase_14_editor_control.TOOL_TESTS),
    ('plugin', phase_15_plugin.TOOL_TESTS),
    ('undo', phase_16_undo.TOOL_TESTS),
    ('csharp', phase_17_csharp.TOOL_TESTS),
    ('cleanup', phase_18_cleanup.TOOL_TESTS),
]

total = sum(len(tests) for _, tests in phases)
print(f"OK: {len(phases)} phases, {total} tool tests")

r = TestResult('test', 'PASS', expected={'x': 1}, actual={'x': 1})
print(f"  TestResult: {r.status}")
pr = PhaseReport(name='x')
pr.results = [TestResult('a', 'PASS'), TestResult('b', 'FAIL')]
print(f"  PhaseReport: pass={pr.passed} fail={pr.failed}")
tr = TestReport()
tr.add_phase(pr)
d = tr.to_dict()
print(f"  TestReport: total={d['summary']['total_tools']} fail={d['summary']['failed']}")
print('ALL CHECKS PASSED')
