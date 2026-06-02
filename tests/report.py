import json
import os
from datetime import datetime
from typing import Any

from test_phases.base import PhaseReport, TestResult


class TestReport:
    def __init__(self):
        self.environment: dict[str, Any] = {}
        self.phases: list[PhaseReport] = []
        self._start_time = datetime.now()

    def set_env(self, **kwargs):
        self.environment.update(kwargs)

    def add_phase(self, report: PhaseReport):
        self.phases.append(report)

    @property
    def total_tools(self) -> int:
        return sum(len(pr.results) for pr in self.phases)

    @property
    def tested(self) -> int:
        return sum(pr.tested for pr in self.phases)

    @property
    def passed(self) -> int:
        return sum(pr.passed for pr in self.phases)

    @property
    def failed(self) -> int:
        return sum(pr.failed for pr in self.phases)

    @property
    def skipped(self) -> int:
        return sum(pr.skipped for pr in self.phases)

    @property
    def total_duration(self) -> float:
        return sum(pr.duration for pr in self.phases)

    def to_dict(self) -> dict:
        return {
            "environment": self.environment,
            "summary": {
                "total_tools": self.total_tools,
                "tested": self.tested,
                "passed": self.passed,
                "failed": self.failed,
                "skipped": self.skipped,
                "duration_seconds": round(self.total_duration, 1),
            },
            "phases": [
                {
                    "name": p.name,
                    "duration": round(p.duration, 1),
                    "tested": p.tested,
                    "passed": p.passed,
                    "failed": p.failed,
                    "skipped": p.skipped,
                    "results": [r.to_dict() for r in p.results],
                }
                for p in self.phases
            ],
            "failures": [
                {
                    "tool": r.tool,
                    "phase": p.name,
                    "expected": TestResult._safe_str(r.expected),
                    "actual": TestResult._safe_str(r.actual),
                    "error": r.error,
                }
                for p in self.phases for r in p.results if r.status == "FAIL"
            ],
        }

    def save_json(self, output_dir: str = "output"):
        timestamp = self._start_time.strftime("%Y%m%d_%H%M%S")
        path = os.path.join(output_dir, f"report-{timestamp}.json")
        os.makedirs(output_dir, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(self.to_dict(), f, ensure_ascii=False, indent=2)
        return path

    def save_markdown(self, output_dir: str = "output"):
        timestamp = self._start_time.strftime("%Y%m%d_%H%M%S")
        path = os.path.join(output_dir, f"report-{timestamp}.md")
        os.makedirs(output_dir, exist_ok=True)

        d = self.to_dict()
        env = d["environment"]
        summary = d["summary"]

        lines = [
            "# GodotMCP Test Report",
            "",
            f"**Date**: {self._start_time.strftime('%Y-%m-%d %H:%M:%S')}",
            "",
            "## Environment",
            "",
            f"| Key | Value |",
            f"|-----|-------|",
        ]
        for k, v in env.items():
            lines.append(f"| **{k}** | {v} |")

        lines += [
            "",
            "## Summary",
            "",
            f"| Metric | Count |",
            f"|--------|-------|",
            f"| Total Tools | {summary['total_tools']} |",
            f"| Tested | {summary['tested']} |",
            f"| Passed | {summary['passed']} |",
            f"| Failed | {summary['failed']} |",
            f"| Skipped | {summary['skipped']} |",
            f"| Duration | {summary['duration_seconds']}s |",
            "",
            "## Results by Phase",
            "",
        ]

        for phase in d["phases"]:
            name = phase["name"].replace("_", " ").title()
            lines += [
                f"### {name}",
                "",
                f"Tested: {phase['tested']} | Passed: {phase['passed']} | Failed: {phase['failed']} | Skipped: {phase['skipped']} | Duration: {phase['duration']}s",
                "",
                "| Tool | Status | Duration |",
                "|------|--------|----------|",
            ]
            for r in phase["results"]:
                icon = {"PASS": "+", "FAIL": "X", "SKIP": "-"}.get(r["status"], "?")
                lines.append(f"| {r['tool']} | {icon} {r['status']} | {r['duration']}s |")
            lines.append("")

        if d["failures"]:
            lines += [
                "## Failed Tools Detail",
                "",
            ]
            for f_ in d["failures"]:
                lines += [
                    f"### X {f_['tool']} ({f_['phase']})",
                    "",
                    f"- **Expected**: `{f_['expected']}`",
                    f"- **Actual**: `{f_['actual']}`",
                    f"- **Error**: {f_['error']}",
                    "",
                ]

        lines.append("")

        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
        return path
