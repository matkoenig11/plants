from __future__ import annotations

from typing import Any

from pydantic import BaseModel, Field


class DeviceTokenCreateRequest(BaseModel):
    device_label: str = Field(min_length=1, max_length=200)


class DeviceTokenCreateResponse(BaseModel):
    token: str
    device_label: str
    created_at: str


class PingResponse(BaseModel):
    status: str
    account_id: int


class SyncRequest(BaseModel):
    client_id: str = Field(min_length=1)
    last_sync_cursor: str | None = None
    changes: dict[str, Any] = Field(default_factory=dict)


class SyncResponse(BaseModel):
    new_cursor: str
    applied: dict[str, int]
    server_changes: dict[str, Any]
    conflicts: list[dict[str, Any]]
