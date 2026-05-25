from typing import Any

from pydantic import BaseModel, Field


class IpcRequest(BaseModel):
    id: str
    method: str
    params: dict[str, Any]


class IpcResponse(BaseModel):
    id: str
    status: str = "ok"
    data: Any = None
    code: int = 0
    message: str = ""

    @classmethod
    def ok(cls, id: str, data: Any = None) -> "IpcResponse":
        return cls(id=id, status="ok", data=data)

    @classmethod
    def error(cls, id: str, code: int, message: str) -> "IpcResponse":
        return cls(id=id, status="error", code=code, message=message)


class IpcNotification(BaseModel):
    msg_type: str = Field(default="notification", alias="type")
    event: str
    data: dict[str, Any]

    model_config = {"populate_by_name": True}


class ToolCallParams(BaseModel):
    tool: str
    args: dict[str, Any] = {}


class ToolInfo(BaseModel):
    name: str
    description: str
    input_schema: dict[str, Any]
    enabled: bool = True


class ToolListUpdate(BaseModel):
    tools: list["ToolState"]


class ToolState(BaseModel):
    name: str
    enabled: bool
