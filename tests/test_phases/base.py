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

    @property
    def duration(self) -> float:
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
