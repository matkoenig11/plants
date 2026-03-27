# Trefle Plant Mapping

This document captures how the current local plant database maps to the Trefle data we inspected for `Astrantia major`, what fits cleanly, what does not fit well, and which fields should remain local or user-managed.

## Current Local Plant Schema

The effective in-app plant model is:

- [Plant.h](c:/Z_Programming_Backup/Programming/plants/core/Plant.h)

Current usable local fields:

- `name`
- `scientificName`
- `plantType`
- `lightRequirement`
- `wateringFrequency`
- `wateringNotes`
- `humidityPreference`
- `soilType`
- `lastWatered`
- `fertilizingSchedule`
- `lastFertilized`
- `pruningTime`
- `pruningNotes`
- `lastPruned`
- `growthRate`
- `issuesPests`
- `temperatureTolerance`
- `toxicToPets`
- `poisonousToHumans`
- `poisonousToPets`
- `indoor`
- `floweringSeason`
- `acquiredDate`
- `source`
- `notes`

## Trefle Astrantia Example

Verified against the saved live payload in:

- [astrantia_trefle_detail.json](c:/Z_Programming_Backup/Programming/plants/tests/API/samples/astrantia_trefle_detail.json)

Important top-level fields returned for `Astrantia major`:

- `common_name`: `Astrantia`
- `scientific_name`: `Astrantia major`
- `family`: `Apiaceae`
- `family_common_name`: `Carrot family`
- `genus`: `Astrantia`
- `rank`: `species`
- `status`: `accepted`
- `observations`: `C. & S. Europe to N. Caucasus`
- `distribution.native`: list of native countries/regions
- `distribution.introduced`: list of introduced countries/regions
- `flower.color`: `["white"]`
- `specifications.growth_habit`: `Forb/herb`
- `specifications.toxicity`: `null`

Important `growth` fields returned:

- `growth.ph_minimum`: `6.5`
- `growth.ph_maximum`: `7.0`
- `growth.light`: `6`
- `growth.atmospheric_humidity`: `5`
- `growth.bloom_months`: `["jun", "jul", "aug"]`
- `growth.soil_nutriments`: `6`
- `growth.soil_salinity`: `0`

Important `growth` fields that were present but empty for Astrantia:

- `growth.description`
- `growth.sowing`
- `growth.minimum_temperature`
- `growth.maximum_temperature`
- `growth.minimum_precipitation`
- `growth.maximum_precipitation`
- `growth.soil_texture`
- `growth.soil_humidity`
- `growth.growth_months`

## Good Mapping

These fields map reasonably well for a first-pass importer.

- `name` <- `common_name`
- `scientificName` <- `scientific_name`
- `plantType` <- `specifications.growth_habit`
- `floweringSeason` <- `growth.bloom_months` joined into one string
- `humidityPreference` <- `growth.atmospheric_humidity` if we define a local scale mapping
- `source` <- fixed string such as `Trefle`
- `notes` <- `observations`, selected distribution data, and other useful descriptive text

## Partial Or Weak Mapping

These can be mapped, but only with interpretation rules instead of a clean direct transfer.

- `lightRequirement` <- `growth.light`
  - Trefle returns a numeric scale, not text such as `full sun` or `part shade`
  - We need a local normalization table if we want user-friendly output
- `soilType` <- `growth.soil_texture`
  - good in theory, but `Astrantia` had `null`
- `temperatureTolerance` <- `growth.minimum_temperature` and `growth.maximum_temperature`
  - good in theory, but `Astrantia` had `null`
- `growthRate` <- `specifications.growth_rate`
  - good in theory, but `Astrantia` had `null`
- `poisonousToHumans` / `poisonousToPets` / `toxicToPets` <- `specifications.toxicity`
  - Trefle only exposes one general toxicity field here, not separate human/pet values
  - `Astrantia` had `null`, so there is nothing useful to import for this example

## Fields We Have Locally But Trefle Astrantia Does Not Really Answer

These should remain local or user-managed instead of being auto-filled from Trefle for now.

- `wateringFrequency`
- `wateringNotes`
- `lastWatered`
- `fertilizingSchedule`
- `lastFertilized`
- `pruningTime`
- `pruningNotes`
- `lastPruned`
- `issuesPests`
- `indoor`
- `acquiredDate`

For this Astrantia payload, Trefle did not provide direct gardening-care fields comparable to:

- Perenual `watering`
- Perenual `sunlight`
- Perenual `soil`
- Perenual `pruning_month`
- Perenual `indoor`
- Perenual `poisonous_to_humans`
- Perenual `poisonous_to_pets`

## Useful Trefle Fields We Cannot Store Properly Today

These are useful fields from Trefle that do not have strong dedicated columns in the current plant model.

- `family_common_name`
- `distribution.native`
- `distribution.introduced`
- `common_names`
- `flower.color`
- `foliage`
- `fruit_or_seed`
- `images`
- `sources`
- `synonyms`
- `growth.ph_minimum`
- `growth.ph_maximum`
- `growth.soil_nutriments`
- `growth.soil_salinity`
- `growth.sowing`
- `growth.spread`
- `growth.row_spacing`

Some of these can be appended to `notes`, but that is only a fallback.

## What Trefle Is Good At

Compared with Perenual, Trefle is better as:

- a botanical/reference source
- a taxonomy source
- a distribution/source-tracking source
- a structured traits/ecology source

It is weaker than Perenual for direct user-facing care defaults like:

- watering
- pruning
- indoor/outdoor suitability
- toxicity split by humans vs pets
- simple sunlight labels

## Practical Import Recommendation

For a first Trefle-backed fill, only auto-fill:

- identity
- growth habit
- flowering season from bloom months
- possibly light and humidity if we define a numeric-scale translation table
- notes
- source

Do not auto-fill:

- watering
- fertilizing
- pruning
- toxicity flags
- indoor
- care-history dates
- acquisition date

## Recommended First Mapping Set

If we implement a `Trefle -> QVariantMap` mapper for the app, this is the safest first output:

- `name`
- `scientificName`
- `plantType`
- `floweringSeason`
- `source`
- `notes`

Optional, if we define explicit local translation rules for numeric scales:

- `lightRequirement`
- `humidityPreference`

Everything else should stay blank unless we extend the local schema or explicitly choose interpretation rules.
