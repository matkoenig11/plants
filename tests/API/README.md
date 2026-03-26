# Garden API Test Tool

This folder contains a small Python UI for testing gardening and plant-search API ideas without changing the Qt app.

It currently supports these plant data sources:

- Trefle
- GBIF
- Perenual
- OpenFarm

Notes:

- GBIF does not need a token.
- OpenFarm's public service was shut down in April 2025, so this option is intended for a self-hosted OpenFarm instance.

## Files

- `garden_api_client.py`
  - provider-based API client
  - scores search results and picks the best fit
- `garden_api_browser.py`
  - Tkinter desktop UI
- `test_garden_api_client.py`
  - mocked unit tests, no live API calls

## Run the UI

From the repo root:

```powershell
python tests\API\garden_api_browser.py
```

Or with a token in the environment:

```powershell
$env:TREFLE_TOKEN="your-token"
python tests\API\garden_api_browser.py
```

Or save tokens and provider settings locally in `tests/API/.env`:

```text
TREFLE_TOKEN=your-token
PERENUAL_TOKEN=your-token
OPENFARM_BASE_URL=http://localhost:3000/api/v1
```

## What the UI does

1. You choose the provider.
2. You type a plant name or partial name such as `orchid`.
3. The tool calls that provider's search endpoint.
4. It ranks the returned candidates.
5. You click any returned possibility.
6. The tool fetches the full detail payload for that exact result.
7. It shows:
   - a readable summary
   - the raw detailed payload

## Run the tests

```powershell
python -m unittest discover -s tests\API -p "test_*.py"
```
