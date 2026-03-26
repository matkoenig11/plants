from __future__ import annotations

from dataclasses import dataclass
import json
import os
from pathlib import Path
from typing import Any, Callable
from urllib.parse import urlencode
from urllib.request import Request, urlopen


ENV_PATH = Path(__file__).with_name(".env")


class PlantApiError(RuntimeError):
    pass


@dataclass(frozen=True)
class ProviderDefinition:
    provider_id: str
    display_name: str
    requires_token: bool = False
    token_env_key: str = ""
    token_label: str = "API token"
    base_url_env_key: str = ""
    default_base_url: str = ""
    supports_custom_base_url: bool = False
    notes: str = ""


@dataclass
class PlantCandidate:
    provider_id: str
    slug: str
    scientific_name: str
    common_name: str
    family: str
    family_common_name: str
    image_url: str
    score: float
    raw: dict[str, Any]


PROVIDERS: dict[str, ProviderDefinition] = {
    "trefle": ProviderDefinition(
        provider_id="trefle",
        display_name="Trefle",
        requires_token=True,
        token_env_key="TREFLE_TOKEN",
        token_label="Trefle token",
        default_base_url="https://trefle.io/api/v1",
        notes="Official plant API with token-based access.",
    ),
    "gbif": ProviderDefinition(
        provider_id="gbif",
        display_name="GBIF",
        default_base_url="https://api.gbif.org/v1",
        notes="No token required. Uses GBIF species search filtered to Plantae.",
    ),
    "perenual": ProviderDefinition(
        provider_id="perenual",
        display_name="Perenual",
        requires_token=True,
        token_env_key="PERENUAL_TOKEN",
        token_label="Perenual token",
        default_base_url="https://perenual.com/api/v2",
        notes="Official plant API with species list and detail endpoints.",
    ),
    "openfarm": ProviderDefinition(
        provider_id="openfarm",
        display_name="OpenFarm",
        base_url_env_key="OPENFARM_BASE_URL",
        supports_custom_base_url=True,
        notes="Public OpenFarm servers were shut down in April 2025. Use a self-hosted base URL.",
    ),
}


def load_local_settings() -> dict[str, str]:
    values: dict[str, str] = {}
    if not ENV_PATH.exists():
        return values

    for line in ENV_PATH.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or "=" not in stripped:
            continue
        key, value = stripped.split("=", 1)
        values[key.strip()] = value.strip()
    return values


def get_provider_definition(provider_id: str) -> ProviderDefinition:
    if provider_id not in PROVIDERS:
        raise PlantApiError(f"Unsupported provider: {provider_id}")
    return PROVIDERS[provider_id]


def provider_choices() -> list[ProviderDefinition]:
    return list(PROVIDERS.values())


def load_provider_token(provider_id: str) -> str:
    provider = get_provider_definition(provider_id)
    if not provider.token_env_key:
        return ""

    settings = load_local_settings()
    env_value = os.getenv(provider.token_env_key, "").strip()
    if env_value:
        return env_value
    return settings.get(provider.token_env_key, "").strip()


def load_provider_base_url(provider_id: str) -> str:
    provider = get_provider_definition(provider_id)
    settings = load_local_settings()
    if provider.base_url_env_key:
        env_value = os.getenv(provider.base_url_env_key, "").strip()
        if env_value:
            return env_value
        file_value = settings.get(provider.base_url_env_key, "").strip()
        if file_value:
            return file_value
    return provider.default_base_url


def normalize(value: str | None) -> str:
    if not value:
        return ""
    return " ".join(value.strip().lower().split())


def build_url(base_url: str, path: str, params: dict[str, Any] | None = None) -> str:
    query = urlencode({key: value for key, value in (params or {}).items() if value is not None and value != ""})
    separator = "&" if "?" in path else "?"
    return f"{base_url.rstrip('/')}{path}{separator + query if query else ''}"


def fetch_json(url: str) -> dict[str, Any]:
    request = Request(url, headers={"Accept": "application/json", "User-Agent": "PlantJournalApiTester/1.0"})
    with urlopen(request, timeout=20) as response:
        charset = response.headers.get_content_charset("utf-8")
        payload = response.read().decode(charset)
    return json.loads(payload)


def score_candidate(query: str, item: dict[str, Any]) -> float:
    q = normalize(query)
    common_name = normalize(item.get("common_name"))
    scientific_name = normalize(item.get("scientific_name"))
    family = normalize(item.get("family"))
    family_common_name = normalize(item.get("family_common_name"))

    if not q:
        return 0.0

    score = 0.0

    def apply_text_score(text: str, exact: float, starts: float, contains: float) -> None:
        nonlocal score
        if not text:
            return
        if text == q:
            score += exact
        elif text.startswith(q):
            score += starts
        elif q in text:
            score += contains

    apply_text_score(common_name, 120.0, 90.0, 60.0)
    apply_text_score(scientific_name, 100.0, 80.0, 55.0)
    apply_text_score(family_common_name, 45.0, 30.0, 15.0)
    apply_text_score(family, 35.0, 25.0, 10.0)

    for token in q.split():
        if token and token in scientific_name:
            score += 8.0
        if token and token in common_name:
            score += 8.0

    if item.get("image_url"):
        score += 3.0
    if item.get("family"):
        score += 2.0
    if item.get("common_name"):
        score += 2.0

    return score


def sort_candidates(candidates: list[PlantCandidate]) -> list[PlantCandidate]:
    return sorted(
        candidates,
        key=lambda candidate: (
            -candidate.score,
            normalize(candidate.common_name or candidate.scientific_name),
            normalize(candidate.scientific_name),
        ),
    )


def parse_trefle_candidates(query: str, payload: dict[str, Any]) -> list[PlantCandidate]:
    candidates: list[PlantCandidate] = []
    for item in payload.get("data", []):
        score = score_candidate(query, item)
        slug = item.get("slug") or str(item.get("id", ""))
        if not slug:
            continue
        candidates.append(
            PlantCandidate(
                provider_id="trefle",
                slug=slug,
                scientific_name=item.get("scientific_name") or "",
                common_name=item.get("common_name") or "",
                family=item.get("family") or "",
                family_common_name=item.get("family_common_name") or "",
                image_url=item.get("image_url") or "",
                score=score,
                raw=item,
            )
        )
    return sort_candidates(candidates)


def parse_candidates(query: str, payload: dict[str, Any]) -> list[PlantCandidate]:
    return parse_trefle_candidates(query, payload)


def parse_gbif_candidates(query: str, payload: dict[str, Any]) -> list[PlantCandidate]:
    candidates: list[PlantCandidate] = []
    for item in payload.get("results", []):
        if item.get("kingdom") and item.get("kingdom") != "Plantae":
            continue
        score_payload = {
            "common_name": item.get("vernacularName") or "",
            "scientific_name": item.get("scientificName") or item.get("canonicalName") or item.get("species") or "",
            "family": item.get("family") or "",
            "family_common_name": item.get("order") or "",
            "image_url": "",
        }
        slug = str(item.get("key") or item.get("speciesKey") or "")
        if not slug:
            continue
        candidates.append(
            PlantCandidate(
                provider_id="gbif",
                slug=slug,
                scientific_name=score_payload["scientific_name"],
                common_name=score_payload["common_name"],
                family=score_payload["family"],
                family_common_name=score_payload["family_common_name"],
                image_url="",
                score=score_candidate(query, score_payload),
                raw=item,
            )
        )
    return sort_candidates(candidates)


def first_string(value: Any) -> str:
    if isinstance(value, list):
        for item in value:
            if item:
                return str(item)
        return ""
    return str(value) if value else ""


def parse_perenual_candidates(query: str, payload: dict[str, Any]) -> list[PlantCandidate]:
    candidates: list[PlantCandidate] = []
    for item in payload.get("data", []):
        default_image = item.get("default_image") or {}
        score_payload = {
            "common_name": item.get("common_name") or "",
            "scientific_name": first_string(item.get("scientific_name")),
            "family": item.get("family") or "",
            "family_common_name": first_string(item.get("other_name")),
            "image_url": default_image.get("original_url") or default_image.get("regular_url") or "",
        }
        slug = str(item.get("id", ""))
        if not slug:
            continue
        candidates.append(
            PlantCandidate(
                provider_id="perenual",
                slug=slug,
                scientific_name=score_payload["scientific_name"],
                common_name=score_payload["common_name"],
                family=score_payload["family"],
                family_common_name=score_payload["family_common_name"],
                image_url=score_payload["image_url"],
                score=score_candidate(query, score_payload),
                raw=item,
            )
        )
    return sort_candidates(candidates)


def normalize_openfarm_base_url(base_url: str) -> str:
    cleaned = base_url.strip().rstrip("/")
    if not cleaned:
        raise PlantApiError(
            "OpenFarm public service was shut down in April 2025. Enter a self-hosted base URL such as "
            "http://localhost:3000/api/v1."
        )
    if not cleaned.endswith("/api/v1"):
        cleaned = f"{cleaned}/api/v1"
    return cleaned


def openfarm_attributes(item: dict[str, Any]) -> dict[str, Any]:
    attributes = item.get("attributes")
    if isinstance(attributes, dict):
        return attributes
    return item


def parse_openfarm_candidates(query: str, payload: dict[str, Any]) -> list[PlantCandidate]:
    candidates: list[PlantCandidate] = []
    data = payload.get("data", [])
    if not isinstance(data, list):
        data = []
    for item in data:
        attributes = openfarm_attributes(item)
        score_payload = {
            "common_name": attributes.get("name") or "",
            "scientific_name": attributes.get("binomial_name") or attributes.get("scientific_name") or "",
            "family": attributes.get("family") or "",
            "family_common_name": "",
            "image_url": attributes.get("main_image_path") or attributes.get("image") or "",
        }
        slug = str(item.get("id") or attributes.get("slug") or attributes.get("name") or "")
        if not slug:
            continue
        candidates.append(
            PlantCandidate(
                provider_id="openfarm",
                slug=slug,
                scientific_name=score_payload["scientific_name"],
                common_name=score_payload["common_name"],
                family=score_payload["family"],
                family_common_name="",
                image_url=score_payload["image_url"],
                score=score_candidate(query, score_payload),
                raw=attributes,
            )
        )
    return sort_candidates(candidates)


def search_candidates(
    query: str,
    provider_id: str,
    *,
    token: str = "",
    base_url: str = "",
    limit: int = 8,
    fetcher: Callable[[str], dict[str, Any]] = fetch_json,
) -> list[PlantCandidate]:
    provider = get_provider_definition(provider_id)
    if not query.strip():
        raise PlantApiError("Enter a plant name or partial name first.")
    if provider.requires_token and not token.strip():
        raise PlantApiError(f"{provider.display_name} requires a token.")

    if provider_id == "trefle":
        url = build_url(provider.default_base_url, "/species/search", {"token": token.strip(), "q": query.strip()})
        payload = fetcher(url)
        return parse_trefle_candidates(query, payload)[:limit]

    if provider_id == "gbif":
        url = build_url(
            provider.default_base_url,
            "/species/search",
            {"q": query.strip(), "rank": "SPECIES", "limit": limit, "highertaxon_key": 6},
        )
        payload = fetcher(url)
        return parse_gbif_candidates(query, payload)[:limit]

    if provider_id == "perenual":
        url = build_url(provider.default_base_url, "/species-list", {"key": token.strip(), "q": query.strip()})
        payload = fetcher(url)
        return parse_perenual_candidates(query, payload)[:limit]

    if provider_id == "openfarm":
        resolved_base_url = normalize_openfarm_base_url(base_url)
        url = build_url(resolved_base_url, "/crops", {"filter": query.strip()})
        payload = fetcher(url)
        return parse_openfarm_candidates(query, payload)[:limit]

    raise PlantApiError(f"Unsupported provider: {provider_id}")


def fetch_candidate_details(
    candidate: PlantCandidate,
    *,
    token: str = "",
    base_url: str = "",
    fetcher: Callable[[str], dict[str, Any]] = fetch_json,
) -> dict[str, Any]:
    provider = get_provider_definition(candidate.provider_id)

    if candidate.provider_id == "trefle":
        url = build_url(provider.default_base_url, f"/species/{candidate.slug}", {"token": token.strip()})
    elif candidate.provider_id == "gbif":
        url = build_url(provider.default_base_url, f"/species/{candidate.slug}")
    elif candidate.provider_id == "perenual":
        url = build_url(provider.default_base_url, f"/species/details/{candidate.slug}", {"key": token.strip()})
    elif candidate.provider_id == "openfarm":
        resolved_base_url = normalize_openfarm_base_url(base_url)
        url = build_url(resolved_base_url, f"/crops/{candidate.slug}")
    else:
        raise PlantApiError(f"Unsupported provider: {candidate.provider_id}")

    payload = fetcher(url)
    if isinstance(payload, dict) and "error" in payload and payload["error"]:
        raise PlantApiError(str(payload["error"]))
    if isinstance(payload, dict) and isinstance(payload.get("data"), dict):
        data = payload["data"]
        if candidate.provider_id == "openfarm":
            return openfarm_attributes(data)
        return data
    return payload


def append_if_present(lines: list[str], label: str, value: Any) -> None:
    if value is None or value == "" or value == []:
        return
    if isinstance(value, list):
        lines.append(f"{label}: {', '.join(str(item) for item in value if item)}")
    else:
        lines.append(f"{label}: {value}")


def build_summary(candidate: PlantCandidate, details: dict[str, Any] | None = None) -> str:
    details = details or {}
    raw = details if details else candidate.raw
    provider = get_provider_definition(candidate.provider_id)

    lines = [
        f"Provider: {provider.display_name}",
        f"Best fit: {candidate.common_name or candidate.scientific_name}",
        f"Identifier: {candidate.slug}",
        f"Scientific name: {candidate.scientific_name or 'Unknown'}",
        f"Common name: {candidate.common_name or 'Unknown'}",
        f"Family: {candidate.family or candidate.family_common_name or 'Unknown'}",
    ]

    append_if_present(lines, "Rank", raw.get("rank"))
    append_if_present(lines, "Status", raw.get("status") or raw.get("taxonomicStatus"))
    append_if_present(lines, "Author", raw.get("author") or raw.get("authorship"))
    append_if_present(lines, "Genus", raw.get("genus"))
    append_if_present(lines, "Order", raw.get("order"))
    append_if_present(lines, "Class", raw.get("class"))
    append_if_present(lines, "Phylum", raw.get("phylum"))
    append_if_present(lines, "Kingdom", raw.get("kingdom"))
    append_if_present(lines, "Cycle", raw.get("cycle"))
    append_if_present(lines, "Watering", raw.get("watering"))
    append_if_present(lines, "Sunlight", raw.get("sunlight"))
    append_if_present(lines, "Other names", raw.get("other_name"))
    append_if_present(lines, "Origin", raw.get("origin"))
    append_if_present(lines, "Duration", raw.get("duration"))
    append_if_present(lines, "Edible", "Yes" if raw.get("edible") is True else "No" if raw.get("edible") is False else None)
    append_if_present(lines, "Vegetable", "Yes" if raw.get("vegetable") is True else "No" if raw.get("vegetable") is False else None)
    append_if_present(lines, "Observations", raw.get("observations"))
    append_if_present(lines, "Description", raw.get("description"))
    append_if_present(lines, "Bibliography", raw.get("bibliography"))

    growth = raw.get("growth")
    if isinstance(growth, dict):
        minimum_temperature = growth.get("minimum_temperature")
        if isinstance(minimum_temperature, dict):
            append_if_present(lines, "Minimum temperature", f"{minimum_temperature.get('deg_c')} C" if minimum_temperature.get("deg_c") is not None else None)
        append_if_present(lines, "Light level", growth.get("light"))
        append_if_present(lines, "Humidity level", growth.get("atmospheric_humidity"))

    if candidate.image_url:
        lines.append(f"Image URL: {candidate.image_url}")

    return "\n".join(lines)


def build_detailed_report(candidate: PlantCandidate, details: dict[str, Any] | None = None) -> str:
    details = details or {}
    sections = [build_summary(candidate, details)]
    sections.append("Full details (raw API payload):")
    sections.append(json.dumps(details or candidate.raw, indent=2, ensure_ascii=False, sort_keys=True))
    return "\n\n".join(sections)


def search_best_fit(
    query: str,
    provider_id: str,
    *,
    token: str = "",
    base_url: str = "",
    fetcher: Callable[[str], dict[str, Any]] = fetch_json,
) -> tuple[PlantCandidate, dict[str, Any], list[PlantCandidate]]:
    candidates = search_candidates(query, provider_id, token=token, base_url=base_url, fetcher=fetcher)
    if not candidates:
        raise PlantApiError("No plant matches were returned.")

    best = candidates[0]
    try:
        details = fetch_candidate_details(best, token=token, base_url=base_url, fetcher=fetcher)
    except PlantApiError:
        details = {}
    return best, details, candidates
