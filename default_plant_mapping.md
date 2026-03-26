# Default Plant Mapping

This document captures how the current local plant database maps to the Perenual data we inspected for `Astrantia major`, what fits cleanly, what does not fit well, and which fields should remain local/user-managed.

## Current Local Plant Schema

The plant schema is defined across these SQL files:

- [001_plants.sql](c:/Z_Programming_Backup/Programming/plants/data/sql/schema/001_plants.sql)
- [101_identity_and_location.sql](c:/Z_Programming_Backup/Programming/plants/data/sql/plants/101_identity_and_location.sql)
- [102_care_profile.sql](c:/Z_Programming_Backup/Programming/plants/data/sql/plants/102_care_profile.sql)
- [103_health_and_source.sql](c:/Z_Programming_Backup/Programming/plants/data/sql/plants/103_health_and_source.sql)

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

## Perenual Astrantia Example

Verified against `Astrantia major` from Perenual:

- `id`: `1101`
- `common_name`: `greater masterwort`
- `scientific_name[0]`: `Astrantia major`
- `other_name[0]`: `astrantia`
- `family`: `Apiaceae`
- `origin[0]`: `Europe`
- `type`: `Herb`
- `cycle`: `Herbaceous Perennial`
- `watering`: `Frequent`
- `watering_general_benchmark.value`: `"7-10"`
- `watering_general_benchmark.unit`: `days`
- `sunlight`: `["full sun", "part shade"]`
- `soil`: `["Well-drained"]`
- `maintenance`: `Moderate`
- `care_level`: `Medium`
- `growth_rate`: `High`
- `pruning_month`: `["February", "May", "May"]`
- `drought_tolerant`: `false`
- `indoor`: `false`
- `flowers`: `true`
- `description`: freeform description text

Care-guide sections were also available:

- `section[].type == "watering"`
- `section[].type == "sunlight"`
- `section[].type == "pruning"`
- `section[].description`

## Good Mapping

These fields map cleanly and should be the first pass for a default importer.

- `name` <- `common_name`
- `scientificName` <- `scientific_name[0]`
- `plantType` <- `type`
- `lightRequirement` <- `sunlight` joined into one string
- `wateringFrequency` <- `watering`
- `wateringNotes` <- `watering_general_benchmark` plus watering care-guide description
- `soilType` <- `soil` joined into one string
- `pruningTime` <- `pruning_month` joined into one string
- `pruningNotes` <- pruning care-guide description
- `growthRate` <- `growth_rate`
- `temperatureTolerance` <- derived from `hardiness.min` and `hardiness.max`
- `toxicToPets` <- `poisonous_to_pets`
- `poisonousToHumans` <- `poisonous_to_humans`
- `poisonousToPets` <- `poisonous_to_pets`
- `indoor` <- `indoor`
- `floweringSeason` <- `flowering_season`
- `source` <- fixed string such as `Perenual`
- `notes` <- `description` and optionally selected care-guide text

## Fields We Have Locally But Perenual Does Not Really Answer

These fields should remain local or user-managed instead of being auto-filled from Perenual.

- `lastWatered`
- `lastFertilized`
- `lastPruned`
- `issuesPests`
- `acquiredDate`

These fields are present in the local schema but were not meaningfully present in the Astrantia payload we reviewed:

- `humidityPreference`
- `fertilizingSchedule`

For now, those should stay blank unless a later provider endpoint gives something better.

## Useful Perenual Fields We Cannot Store Properly Today

These are useful fields from Perenual that do not have strong dedicated columns in the current plant model.

- `cycle`
- `origin`
- `maintenance`
- `care_level`
- `propagation`
- `hardiness.min`
- `hardiness.max`
- `drought_tolerant`
- `salt_tolerant`
- `thorny`
- `invasive`
- `indoor`
- `poisonous_to_humans`
- `flowering_season`
- `dimensions`
- `plant_anatomy`
- structured `care guide` sections

Some of these can be appended to `notes`, but that is only a temporary fallback.

## Weak Or Legacy Local Fields

These columns still exist in the SQL schema but do not fit the active model well anymore:

- `species`
- `location`

They come from the original base table in [001_plants.sql](c:/Z_Programming_Backup/Programming/plants/data/sql/schema/001_plants.sql), but the effective app model in [Plant.h](c:/Z_Programming_Backup/Programming/plants/core/Plant.h) no longer uses them.

They have effectively been replaced by:

- `scientificName`
- `notes`

Unless there is a hidden dependency elsewhere, these look like legacy baggage.

## Practical Import Recommendation

For a first Perenual-backed default fill, auto-fill only:

- identity
- light
- soil
- watering
- pruning
- growth
- temperature tolerance
- pet toxicity
- descriptive notes
- source

Do not auto-fill:

- local placement
- local care-history dates
- fertilizing schedule
- health state
- acquisition date

## Recommended First Mapping Set

If we implement a `Perenual -> QVariantMap` mapper for the app, this is the recommended first output:

- `name`
- `scientificName`
- `plantType`
- `lightRequirement`
- `wateringFrequency`
- `wateringNotes`
- `soilType`
- `pruningTime`
- `pruningNotes`
- `growthRate`
- `temperatureTolerance`
- `toxicToPets`
- `poisonousToHumans`
- `poisonousToPets`
- `indoor`
- `floweringSeason`
- `source`
- `notes`

Everything else should remain untouched or blank until we decide whether to extend the local schema.
