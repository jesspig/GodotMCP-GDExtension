from dataclasses import dataclass, field
from typing import Any


@dataclass
class TestResult:
    tool: str
    status: str
    expected: Any = None
    actual: Any = None
    error: str = ""
    duration: float = 0.0

    def to_dict(self) -> dict:
        return {
            "tool": self.tool,
            "status": self.status,
            "expected": self._safe_str(self.expected),
            "actual": self._safe_str(self.actual),
            "error": self.error,
            "duration": round(self.duration, 3),
        }

    @staticmethod
    def _safe_str(v: Any) -> str:
        if v is None:
            return ""
        try:
            s = str(v)
            return s[:500] + "..." if len(s) > 500 else s
        except Exception:
            return "<unprintable>"


@dataclass
class PhaseReport:
    name: str
    results: list[TestResult] = field(default_factory=list)
    start_time: float = 0.0
    end_time: float = 0.0
    call_count: int = 0
    call_success: int = 0
    call_fail: int = 0
    call_skip: int = 0
    unique_tools: int = 0
    unique_success: int = 0
    unique_fail: int = 0
    unique_skip: int = 0
    duration_ms: int = 0
    errors: list = field(default_factory=list)

    @property
    def duration(self) -> float:
        if self.duration_ms > 0:
            return self.duration_ms / 1000.0
        return self.end_time - self.start_time if self.end_time > self.start_time else 0.0

    @property
    def passed(self) -> int:
        return sum(1 for r in self.results if r.status == "PASS")

    @property
    def failed(self) -> int:
        return sum(1 for r in self.results if r.status == "FAIL")

    @property
    def skipped(self) -> int:
        return sum(1 for r in self.results if r.status == "SKIP")

    @property
    def tested(self) -> int:
        return self.passed + self.failed

    def to_dict(self) -> dict:
        return {
            "name": self.name,
            "duration": round(self.duration, 1),
            "tested": self.tested,
            "passed": self.passed,
            "failed": self.failed,
            "skipped": self.skipped,
            "call_count": self.call_count,
            "call_success": self.call_success,
            "call_fail": self.call_fail,
            "call_skip": self.call_skip,
            "unique_tools": self.unique_tools,
            "unique_success": self.unique_success,
            "unique_fail": self.unique_fail,
            "unique_skip": self.unique_skip,
            "duration_ms": self.duration_ms,
            "errors": self.errors,
            "results": [r.to_dict() for r in self.results],
        }
