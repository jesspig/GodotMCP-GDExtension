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
    def total_call_count(self) -> int:
        return sum(pr.call_count for pr in self.phases)

    @property
    def total_call_success(self) -> int:
        return sum(pr.call_success for pr in self.phases)

    @property
    def total_call_fail(self) -> int:
        return sum(pr.call_fail for pr in self.phases)

    @property
    def total_call_skip(self) -> int:
        return sum(pr.call_skip for pr in self.phases)

    @property
    def total_unique_tools(self) -> int:
        return len({r.tool for pr in self.phases for r in pr.results})

    @property
    def total_unique_success(self) -> int:
        return len({r.tool for pr in self.phases for r in pr.results if r.status == "PASS"})

    @property
    def total_unique_fail(self) -> int:
        return len({r.tool for pr in self.phases for r in pr.results if r.status == "FAIL"})

    @property
    def total_unique_skip(self) -> int:
        return len({r.tool for pr in self.phases for r in pr.results if r.status == "SKIP"})

    @property
    def tool_matrix(self) -> dict[str, list[dict]]:
        """Per-tool cross-pipeline matrix.
        Returns {tool_name: [{phase, status, error}, ...], ...}
        Only includes actual test steps (not setup/teardown chains).
        """
        matrix: dict[str, list[dict]] = {}
        for pr in self.phases:
            for r in pr.results:
                if r.tool not in matrix:
                    matrix[r.tool] = []
                matrix[r.tool].append({
                    "phase": pr.name,
                    "status": r.status,
                    "error": r.error if r.status != "PASS" else "",
                })
        return matrix

    @property
    def tool_summary(self) -> list[dict]:
        """Aggregated per-tool stats across all phases.
        Each entry: {tool, phases, total, pass, fail, skip, failing_phases}
        """
        raw: dict[str, dict] = {}
        for t, entries in self.tool_matrix.items():
            phase_list = [e["phase"] for e in entries]
            fail_phases = [e["phase"] for e in entries if e["status"] == "FAIL"]
            total = len(entries)
            passed = sum(1 for e in entries if e["status"] == "PASS")
            failed = sum(1 for e in entries if e["status"] == "FAIL")
            skipped = sum(1 for e in entries if e["status"] == "SKIP")
            raw[t] = {
                "tool": t,
                "phases": total,
                "total": total,
                "pass": passed,
                "fail": failed,
                "skip": skipped,
                "failing_phases": fail_phases,
            }
        return sorted(raw.values(), key=lambda x: (-x["fail"], -x["phases"], x["tool"]))

    @property
    def all_errors(self) -> list[dict]:
        seen = set()
        unique = []
        for p in self.phases:
            for err in p.errors:
                key = (err.get("tool", ""), err.get("error", ""))
                if key not in seen:
                    seen.add(key)
                    unique.append(err)
        return unique

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
                "total_call_count": self.total_call_count,
                "total_call_success": self.total_call_success,
                "total_call_fail": self.total_call_fail,
                "total_call_skip": self.total_call_skip,
                "total_unique_tools": self.total_unique_tools,
                "total_unique_success": self.total_unique_success,
                "total_unique_fail": self.total_unique_fail,
                "total_unique_skip": self.total_unique_skip,
                "unique_errors": len(self.all_errors),
            },
            "phases": [p.to_dict() for p in self.phases],
            "tool_matrix": self.tool_matrix,
            "tool_summary": self.tool_summary,
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
            f"| Total Calls | {summary['total_call_count']} |",
            f"| Call Success | {summary['total_call_success']} |",
            f"| Call Fail | {summary['total_call_fail']} |",
            f"| Call Skip | {summary['total_call_skip']} |",
            f"| Unique Tools | {summary['total_unique_tools']} |",
            f"| Unique Success | {summary['total_unique_success']} |",
            f"| Unique Fail | {summary['total_unique_fail']} |",
            f"| Unique Skip | {summary['total_unique_skip']} |",
            f"| Unique Errors | {summary['unique_errors']} |",
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

        all_errors = self.all_errors
        # --- Tool Cross-Pipeline Matrix ---
        tool_summary = d.get("tool_summary", [])
        if tool_summary:
            lines += [
                "## Tool Cross-Pipeline Matrix",
                "",
                "| Tool | Phases | Pass | Fail | Skip | Details |",
                "|------|--------|------|------|------|---------|",
            ]
            for ts in tool_summary:
                fail_phases = ",".join(ts["failing_phases"]) if ts["fail"] > 0 else ""
                line = f"| {ts['tool']} | {ts['phases']} | {ts['pass']} | {ts['fail']} | {ts['skip']} | {fail_phases} |"
                lines.append(line)
            lines.append("")

        if all_errors:
            lines += [
                "## Error Summary",
                "",
                "| Tool | Error |",
                "|------|-------|",
            ]
            for err in all_errors:
                tool = err.get("tool", "")
                error = err.get("error", "")
                lines.append(f"| {tool} | {error} |")
            lines.append("")

        lines.append("")

        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
        return path
