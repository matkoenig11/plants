from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import quote


@dataclass(frozen=True)
class Settings:
    database_url: str
    admin_secret: str
    default_account_slug: str = "default"
    sql_dir: Path = Path(__file__).resolve().parents[1] / "sql"


def load_env_file(env_path: Path) -> None:
    if not env_path.exists():
        return

    for raw_line in env_path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue

        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip().strip("'").strip('"')
        if key and key not in os.environ:
            os.environ[key] = value


def build_database_url_from_parts() -> str:
    host = os.getenv("POSTGRES_HOST", "").strip()
    port = os.getenv("POSTGRES_PORT", "5432").strip() or "5432"
    database = os.getenv("POSTGRES_DB", "").strip()
    user = os.getenv("POSTGRES_USER", "").strip()
    password = os.getenv("POSTGRES_PASSWORD", "")
    sslmode = os.getenv("POSTGRES_SSLMODE", "").strip()

    if not (host and database and user):
        return ""

    encoded_user = quote(user, safe="")
    encoded_password = quote(password, safe="")
    auth_part = encoded_user
    if password:
        auth_part += f":{encoded_password}"

    url = f"postgresql://{auth_part}@{host}:{port}/{database}"
    if sslmode:
        url += f"?sslmode={quote(sslmode, safe='')}"
    return url


def load_settings() -> Settings:
    repo_root = Path(__file__).resolve().parents[2]
    load_env_file(repo_root / "data" / "configurations" / "postgres" / ".env")

    database_url = os.getenv("PLANT_JOURNAL_DATABASE_URL", "").strip()
    if not database_url:
        database_url = build_database_url_from_parts()
    admin_secret = os.getenv("PLANT_JOURNAL_ADMIN_SECRET", "").strip()
    account_slug = os.getenv("PLANT_JOURNAL_DEFAULT_ACCOUNT", "default").strip() or "default"
    if not database_url:
        raise RuntimeError(
            "PLANT_JOURNAL_DATABASE_URL is required, or POSTGRES_HOST/PORT/DB/USER/PASSWORD must be set."
        )
    if not admin_secret:
        raise RuntimeError("PLANT_JOURNAL_ADMIN_SECRET is required.")
    return Settings(
        database_url=database_url,
        admin_secret=admin_secret,
        default_account_slug=account_slug,
    )
