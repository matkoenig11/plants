from __future__ import annotations

from fastapi.testclient import TestClient


class FakeSyncService:
    def __init__(self) -> None:
        self.created_label = None
        self.authenticated_token = None
        self.synced_payload = None

    def create_device_token(self, device_label: str) -> dict:
        self.created_label = device_label
        return {
            "token": "issued-token",
            "device_label": device_label,
            "created_at": "2026-03-26T12:00:00Z",
        }

    def authenticate(self, token: str) -> int | None:
        self.authenticated_token = token
        return 1 if token == "valid-token" else None

    def sync(self, account_id: int, payload: dict) -> dict:
        self.synced_payload = payload
        return {
            "new_cursor": "2026-03-26T13:00:00Z",
            "applied": {
                "plants": 1,
                "journal_entries": 0,
                "reminders": 0,
                "plant_images": 0,
                "plant_care_schedules": 0,
                "reminder_settings": 0,
                "tombstones": 0,
            },
            "server_changes": {
                "plants": [],
                "journal_entries": [],
                "reminders": [],
                "plant_images": [],
                "plant_care_schedules": [],
                "reminder_settings": None,
                "tombstones": [],
            },
            "conflicts": [],
        }


def build_client(monkeypatch):
    monkeypatch.setenv("PLANT_JOURNAL_DATABASE_URL", "postgresql://unused")
    monkeypatch.setenv("PLANT_JOURNAL_ADMIN_SECRET", "secret")

    from server.app import main

    main.get_settings.cache_clear()
    main.get_sync_service.cache_clear()
    monkeypatch.setattr(main, "apply_migrations", lambda settings: None)

    fake_service = FakeSyncService()
    main.app.dependency_overrides[main.get_sync_service] = lambda: fake_service
    client = TestClient(main.app)
    return client, fake_service, main


def test_ping_requires_bearer_token(monkeypatch):
    client, _, main = build_client(monkeypatch)
    response = client.get("/api/v1/ping")
    assert response.status_code == 401
    main.app.dependency_overrides.clear()


def test_ping_success(monkeypatch):
    client, fake_service, main = build_client(monkeypatch)
    response = client.get(
        "/api/v1/ping",
        headers={"Authorization": "Bearer valid-token"},
    )
    assert response.status_code == 200
    body = response.json()
    assert body["status"] == "ok"
    assert body["account_id"] == 1
    assert fake_service.authenticated_token == "valid-token"
    main.app.dependency_overrides.clear()


def test_sync_requires_bearer_token(monkeypatch):
    client, _, main = build_client(monkeypatch)
    response = client.post("/api/v1/sync", json={"client_id": "c1", "changes": {}})
    assert response.status_code == 401
    main.app.dependency_overrides.clear()


def test_device_token_requires_admin_secret(monkeypatch):
    client, _, main = build_client(monkeypatch)
    response = client.post(
        "/api/v1/device-tokens",
        json={"device_label": "Phone"},
        headers={"X-Admin-Secret": "wrong"},
    )
    assert response.status_code == 401
    main.app.dependency_overrides.clear()


def test_sync_success(monkeypatch):
    client, fake_service, main = build_client(monkeypatch)
    response = client.post(
        "/api/v1/sync",
        json={"client_id": "c1", "last_sync_cursor": None, "changes": {}},
        headers={"Authorization": "Bearer valid-token"},
    )
    assert response.status_code == 200
    body = response.json()
    assert body["new_cursor"] == "2026-03-26T13:00:00Z"
    assert fake_service.authenticated_token == "valid-token"
    assert fake_service.synced_payload["client_id"] == "c1"
    main.app.dependency_overrides.clear()
