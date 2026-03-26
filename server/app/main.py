from __future__ import annotations

from contextlib import asynccontextmanager
from functools import lru_cache

from fastapi import Depends, FastAPI, Header, HTTPException, status

from .config import Settings, load_settings
from .database import apply_migrations
from .schemas import (
    DeviceTokenCreateRequest,
    DeviceTokenCreateResponse,
    PingResponse,
    SyncRequest,
    SyncResponse,
)
from .service import PostgresSyncService, SyncValidationError


@lru_cache(maxsize=1)
def get_settings() -> Settings:
    return load_settings()


@lru_cache(maxsize=1)
def get_sync_service() -> PostgresSyncService:
    settings = get_settings()
    return PostgresSyncService(settings)


def bearer_token(authorization: str = Header(default="")) -> str:
    if not authorization.startswith("Bearer "):
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Missing bearer token.")
    token = authorization[len("Bearer ") :].strip()
    if not token:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Missing bearer token.")
    return token


@asynccontextmanager
async def lifespan(_: FastAPI):
    apply_migrations(get_settings())
    yield


app = FastAPI(title="Plant Journal Sync API", version="1.0.0", lifespan=lifespan)


@app.post("/api/v1/device-tokens", response_model=DeviceTokenCreateResponse)
def create_device_token(
    request: DeviceTokenCreateRequest,
    admin_secret: str = Header(default="", alias="X-Admin-Secret"),
    service: PostgresSyncService = Depends(get_sync_service),
) -> DeviceTokenCreateResponse:
    if admin_secret != get_settings().admin_secret:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid admin secret.")
    return DeviceTokenCreateResponse.model_validate(service.create_device_token(request.device_label))


@app.get("/api/v1/ping", response_model=PingResponse)
def ping(
    token: str = Depends(bearer_token),
    service: PostgresSyncService = Depends(get_sync_service),
) -> PingResponse:
    account_id = service.authenticate(token)
    if account_id is None:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid device token.")
    return PingResponse(status="ok", account_id=account_id)


@app.post("/api/v1/sync", response_model=SyncResponse)
def sync(
    request: SyncRequest,
    token: str = Depends(bearer_token),
    service: PostgresSyncService = Depends(get_sync_service),
) -> SyncResponse:
    account_id = service.authenticate(token)
    if account_id is None:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid device token.")

    try:
        response = service.sync(account_id, request.model_dump())
    except SyncValidationError as exc:
        raise HTTPException(status_code=exc.status_code, detail=exc.detail) from exc

    return SyncResponse.model_validate(response)
