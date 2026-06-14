import asyncio
import os
import subprocess
import time

import httpx


class GodotManager:
    def __init__(
        self,
        godot_path: str,
        project_path: str = "example",
        headless: bool = True,
        mcp_port: int = 9600,
    ):
        self.godot_path = godot_path
        self.project_path = os.path.abspath(project_path)
        self.headless = headless
        self.mcp_port = mcp_port
        self.mcp_url = f"http://127.0.0.1:{mcp_port}/mcp"
        self.process: subprocess.Popen | None = None
        self._started_by_us = False

    async def ensure_running(self, timeout: int = 60) -> bool:
        if await self._check_mcp_ready():
            self._started_by_us = False
            return True
        return await self._start(timeout)

    async def stop(self, timeout: int = 15):
        if self.process and self.process.poll() is None:
            try:
                self.process.terminate()
                self.process.wait(timeout)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(5)
            except Exception:
                if self.process.poll() is None:
                    self.process.kill()
                    self.process.wait(5)
        self.process = None

    async def _start(self, timeout: int) -> bool:
        if not os.path.isfile(self.godot_path):
            raise FileNotFoundError(f"Godot executable not found: {self.godot_path}")

        if not os.path.isdir(self.project_path):
            raise NotADirectoryError(f"Project not found: {self.project_path}")

        godot_project_file = os.path.join(self.project_path, "project.godot")
        if not os.path.isfile(godot_project_file):
            raise FileNotFoundError(f"No project.godot in {self.project_path}")

        cmd = [self.godot_path, "--editor", "--path", self.project_path]
        if self.headless:
            cmd.append("--headless")

        self.process = subprocess.Popen(
            cmd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0,
        )
        self._started_by_us = True

        if await self._wait_for_mcp(timeout):
            return True

        if self.headless:
            await self.stop()
            self.headless = False
            return await self._start(timeout)

        return False

    async def _wait_for_mcp(self, timeout: int) -> bool:
        deadline = time.time() + timeout
        while time.time() < deadline:
            if self.process and self.process.poll() is not None:
                return False
            if await self._check_mcp_ready():
                return True
            await asyncio.sleep(0.5)
        return False

    async def _check_mcp_ready(self) -> bool:
        try:
            async with httpx.AsyncClient() as client:
                resp = await client.post(
                    self.mcp_url,
                    json={
                        "jsonrpc": "2.0",
                        "id": "ping-1",
                        "method": "ping",
                    },
                    headers={"Accept": "application/json, text/event-stream"},
                    timeout=3,
                )
                return resp.status_code == 200
        except Exception:
            return False

    @property
    def is_running(self) -> bool:
        return self.process is not None and self.process.poll() is None

    def __del__(self):
        if self._started_by_us and self.process and self.process.poll() is None:
            try:
                self.process.terminate()
            except Exception:
                pass
