import unittest

from garden_api_client import (
    PlantApiError,
    build_detailed_report,
    build_summary,
    parse_candidates,
    parse_gbif_candidates,
    parse_perenual_candidates,
    provider_choices,
    score_candidate,
    search_best_fit,
    search_candidates,
)


class GardenApiClientTests(unittest.TestCase):
    def test_provider_choices_include_expected_sources(self) -> None:
        provider_ids = {provider.provider_id for provider in provider_choices()}
        self.assertEqual({"trefle", "gbif", "perenual", "openfarm"}, provider_ids)

    def test_score_candidate_prefers_exact_common_name(self) -> None:
        orchid = {"common_name": "orchid", "scientific_name": "Orchis mascula", "family": "Orchidaceae"}
        cabbage = {"common_name": "cabbage orchid", "scientific_name": "Eulophia", "family": "Orchidaceae"}

        self.assertGreater(score_candidate("orchid", orchid), score_candidate("orchid", cabbage))

    def test_trefle_search_best_fit_fetches_detail_payload(self) -> None:
        urls = []

        def fake_fetcher(url: str):
            urls.append(url)
            if "/species/search" in url:
                return {
                    "data": [
                        {
                            "slug": "phalaenopsis-aphrodite",
                            "common_name": "moon orchid",
                            "scientific_name": "Phalaenopsis aphrodite",
                            "family": "Orchidaceae",
                            "image_url": "https://example.test/orchid.jpg",
                        }
                    ]
                }
            return {
                "data": {
                    "slug": "phalaenopsis-aphrodite",
                    "common_name": "moon orchid",
                    "scientific_name": "Phalaenopsis aphrodite",
                    "family": "Orchidaceae",
                    "growth": {
                        "light": 7,
                        "minimum_temperature": {"deg_c": 18},
                    },
                }
            }

        best, details, candidates = search_best_fit("orchid", "trefle", token="token-123", fetcher=fake_fetcher)

        self.assertEqual("phalaenopsis-aphrodite", best.slug)
        self.assertEqual("Phalaenopsis aphrodite", details["scientific_name"])
        self.assertEqual(2, len(urls))
        self.assertEqual(1, len(candidates))

    def test_gbif_candidates_filter_to_plants(self) -> None:
        payload = {
            "results": [
                {
                    "key": 1,
                    "scientificName": "Orchidvirus orchid",
                    "kingdom": "Heunggongvirae",
                    "family": "Unknown",
                },
                {
                    "key": 2,
                    "scientificName": "Orchis mascula",
                    "vernacularName": "early-purple orchid",
                    "kingdom": "Plantae",
                    "family": "Orchidaceae",
                },
            ]
        }

        candidates = parse_gbif_candidates("orchid", payload)

        self.assertEqual(1, len(candidates))
        self.assertEqual("gbif", candidates[0].provider_id)
        self.assertEqual("2", candidates[0].slug)

    def test_perenual_candidates_parse_image_and_names(self) -> None:
        payload = {
            "data": [
                {
                    "id": 55,
                    "common_name": "moth orchid",
                    "scientific_name": ["Phalaenopsis aphrodite"],
                    "family": "Orchidaceae",
                    "other_name": ["moon orchid"],
                    "default_image": {"original_url": "https://example.test/perenual.jpg"},
                }
            ]
        }

        candidates = parse_perenual_candidates("orchid", payload)

        self.assertEqual("perenual", candidates[0].provider_id)
        self.assertEqual("55", candidates[0].slug)
        self.assertEqual("https://example.test/perenual.jpg", candidates[0].image_url)

    def test_openfarm_requires_custom_base_url(self) -> None:
        with self.assertRaises(PlantApiError):
            search_candidates("tomato", "openfarm")

    def test_build_summary_includes_provider_and_growth_fields(self) -> None:
        candidate = parse_candidates(
            "orchid",
            {
                "data": [
                    {
                        "slug": "phalaenopsis-aphrodite",
                        "common_name": "moon orchid",
                        "scientific_name": "Phalaenopsis aphrodite",
                        "family": "Orchidaceae",
                    }
                ]
            },
        )[0]

        summary = build_summary(
            candidate,
            {
                "growth": {
                    "light": 6,
                    "minimum_temperature": {"deg_c": 16},
                },
                "duration": ["perennial"],
            },
        )

        self.assertIn("Provider: Trefle", summary)
        self.assertIn("Minimum temperature: 16 C", summary)
        self.assertIn("Light level: 6", summary)
        self.assertIn("Duration: perennial", summary)

    def test_build_detailed_report_includes_raw_payload(self) -> None:
        candidate = parse_candidates(
            "orchid",
            {
                "data": [
                    {
                        "slug": "phalaenopsis-aphrodite",
                        "common_name": "moon orchid",
                        "scientific_name": "Phalaenopsis aphrodite",
                        "family": "Orchidaceae",
                    }
                ]
            },
        )[0]

        report = build_detailed_report(
            candidate,
            {
                "slug": "phalaenopsis-aphrodite",
                "growth": {"light": 6},
                "bibliography": "Example source",
            },
        )

        self.assertIn("Full details (raw API payload):", report)
        self.assertIn("\"bibliography\": \"Example source\"", report)
        self.assertIn("\"light\": 6", report)


if __name__ == "__main__":
    unittest.main()
