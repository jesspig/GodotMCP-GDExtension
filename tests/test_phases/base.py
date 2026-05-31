from dataclasses import dataclass, field
from typing import Any, Callable, Awaitable


@dataclass
class TestResult:
    tool: str
    status: str  # PASS / FAIL / SKIP
    expected: Any = None
    actual: Any = None
    error: str = ""
    disk_verified: bool = False
    disk_detail: str = ""
    duration: float = 0.0

    def to_dict(self) -> dict:
        return {
            "tool": self.tool,
            "status": self.status,
            "expected": self._safe_str(self.expected),
            "actual": self._safe_str(self.actual),
            "error": self.error,
            "disk_verified": self.disk_verified,
            "disk_detail": self.disk_detail,
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
class ToolTest:
    name: str
    run: Callable[..., Awaitable[TestResult]]
    setup: Callable[..., Awaitable[None]] | None = None
    teardown: Callable[..., Awaitable[None]] | None = None
    requires_dotnet: bool = False
    always_run: bool = False


class PhaseRunner:
    def __init__(self, ctx, tests: list[ToolTest]):
        self.ctx = ctx
        self.tests = tests

    async def run_all(self) -> list[TestResult]:
        results = []
        for tt in self.tests:
            if not tt.always_run and tt.name not in self.ctx.available_tools:
                results.append(TestResult(tt.name, "SKIP", error="tools/list 中未发现该工具"))
                continue
            if tt.requires_dotnet and not self.ctx.dotnet_available:
                results.append(TestResult(tt.name, "SKIP", error="需要 .NET >= 8.0"))
                continue
            try:
                if tt.setup:
                    await tt.setup(self.ctx)
                import time as _time
                t0 = _time.time()
                result = await tt.run(self.ctx)
                result.duration = _time.time() - t0
                results.append(result)
            except Exception as e:
                import traceback
                tb = traceback.format_exc()
                results.append(TestResult(tt.name, "FAIL", error=f"{e}\n{tb}"))
            finally:
                if tt.teardown:
                    try:
                        await tt.teardown(self.ctx)
                    except Exception:
                        pass
        return results


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
