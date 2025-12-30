-- Seed plants from plants_populated.xlsx (one-time if table empty)
INSERT INTO plants (name, scientific_name, plant_type, winter_location, summer_location, light_requirement, watering_frequency, watering_notes, humidity_preference, soil_type, pot_size, last_watered, fertilizing_schedule, last_fertilized, pruning_time, pruning_notes, last_pruned, growth_rate, current_health_status, issues_pests, temperature_tolerance, toxic_to_pets, acquired_on, source, notes)
SELECT * FROM (
    SELECT 'Calathea 1', 'Calathea (unknown sp.)', 'Houseplant', 'Indoors', NULL, 'Bright indirect', 'Keep lightly moist; 1-2x/week (adjust)', NULL, 'High', NULL, NULL, NULL, NULL, NULL, 'As needed', 'Remove yellow/brown leaves at base', NULL, NULL, 'Unknown', NULL, 'Warm; avoid drafts', 'Likely non-toxic (verify)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Calathea 2', 'Calathea (unknown sp.)', 'Houseplant', 'Indoors', NULL, 'Bright indirect', 'Keep lightly moist; 1-2x/week (adjust)', NULL, 'High', NULL, NULL, NULL, NULL, NULL, 'As needed', 'Remove yellow/brown leaves at base', NULL, NULL, 'Unknown', NULL, 'Warm; avoid drafts', 'Likely non-toxic (verify)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Fiddle Leaf Fig', 'Ficus lyrata', 'Houseplant', 'Indoors', NULL, 'Bright indirect (some sun ok)', 'When top 3-5 cm dry (often 7-14 days)', NULL, 'Medium', NULL, NULL, NULL, NULL, NULL, 'Spring/summer', 'Shape lightly; remove damaged leaves', NULL, NULL, 'Unknown', NULL, 'Warm; avoid cold shock', 'Yes', NULL, NULL, NULL
    UNION ALL
    SELECT 'Lemon Tree', 'Citrus limon', 'Potted tree', 'Indoors (brightest spot)', 'Outdoors (after frost)', 'Full sun / very bright', 'When top soil dries; more in summer', NULL, 'Medium', NULL, NULL, NULL, NULL, NULL, 'Late winter / after harvest (light)', 'Remove dead/crossing branches; shape', NULL, NULL, 'Unknown', NULL, 'Frost sensitive', 'Yes (citrus oils)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Yucca', 'Yucca (likely Y. elephantipes)', 'Houseplant', 'Indoors', 'Outdoors (optional)', 'Bright light; tolerates some sun', 'Let dry well; every 2-4 weeks', NULL, 'Low', NULL, NULL, NULL, NULL, NULL, 'As needed', 'Remove dead lower leaves; cut cane to branch', NULL, NULL, 'Unknown', NULL, 'Tolerates cooler; avoid frost', 'Yes', NULL, NULL, NULL
    UNION ALL
    SELECT 'Orchid', 'Phalaenopsis (likely)', 'Houseplant', 'Indoors', NULL, 'Bright indirect', 'Soak & drain every 7-14 days', NULL, 'Medium-High', NULL, NULL, NULL, NULL, NULL, 'After flowering', 'Cut spike above node or to base depending on health', NULL, NULL, 'Unknown', NULL, 'Warm; avoid cold drafts', 'No (generally)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Ficus (Unknown)', 'Ficus (unknown sp.)', 'Houseplant', 'Indoors', NULL, 'Bright indirect', 'When top soil dries (7-14 days)', NULL, 'Medium', NULL, NULL, NULL, NULL, NULL, 'Spring/summer', 'Pinch/trim for shape; remove dead leaves', NULL, NULL, 'Unknown', NULL, 'Warm; avoid drafts', 'Yes', NULL, NULL, NULL
    UNION ALL
    SELECT 'Bonsai Ficus', 'Ficus microcarpa', 'Bonsai', 'Indoors (bright)', 'Outdoors (warm, sheltered)', 'Very bright; some sun', 'When surface just starts to dry (often 2-7 days)', NULL, 'Medium', NULL, NULL, NULL, NULL, NULL, 'Growing season', 'Pinch back new growth; structural pruning lightly', NULL, NULL, 'Unknown', NULL, 'Warm; no frost', 'Yes', NULL, NULL, NULL
    UNION ALL
    SELECT 'Unknown Plant', 'Unknown', 'Houseplant', 'Indoors', NULL, 'Unknown', 'Unknown', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'Unknown', NULL, NULL, NULL, NULL, NULL, NULL
    UNION ALL
    SELECT 'Dwarf Cavendish Banana', 'Musa acuminata ''Dwarf Cavendish''', 'Tropical', 'Indoors (very bright)', 'Outdoors (warm, sheltered)', 'Full sun / very bright', 'Keep evenly moist; don''t let dry out', NULL, 'High', NULL, NULL, NULL, NULL, NULL, 'As needed', 'Remove damaged leaves; cut dead leaf stems', NULL, NULL, 'Unknown', NULL, 'Cold sensitive; no frost', 'No (generally)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Camellia', 'Camellia (likely japonica)', 'Shrub (potted)', 'Indoors (cool bright) / sheltered', 'Outdoors (partial shade)', 'Bright light; avoid hot midday sun', 'Keep evenly moist; don''t dry out', NULL, 'Medium', 'Acidic (ericaceous)', NULL, NULL, NULL, NULL, 'After flowering', 'Light prune to shape; remove dead wood', NULL, NULL, 'Unknown', NULL, 'Hardy-ish; pot needs protection', 'No (generally)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Olive Tree', 'Olea europaea', 'Potted tree', 'Outdoors (protected) / unheated shelter', 'Outdoors', 'Full sun', 'Let top soil dry; drought tolerant once established', NULL, 'Low', NULL, NULL, NULL, NULL, NULL, 'Late winter / early spring', 'Thin lightly; remove suckers; shape', NULL, NULL, 'Unknown', NULL, 'Tolerates cool; protect from hard frost in pot', 'No (generally)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Lavender', 'Lavandula (unknown sp.)', 'Herb (potted)', 'Outdoors (very well-drained)', 'Outdoors', 'Full sun', 'Let dry; water sparingly', NULL, 'Low', NULL, NULL, NULL, NULL, NULL, 'After flowering / spring', 'Trim lightly; avoid cutting into old wood', NULL, NULL, 'Unknown', NULL, 'Hardy depending on species; pot needs drainage', 'No/low (verify)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Sage', 'Salvia officinalis', 'Herb (potted)', 'Outdoors (protected)', 'Outdoors', 'Full sun', 'When dry; moderate', NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'Spring', 'Cut back lightly; remove woody stems gradually', NULL, NULL, 'Unknown', NULL, 'Hardy; protect pot from deep freeze', 'No/low (culinary)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Rosemary', 'Salvia rosmarinus', 'Herb (potted)', 'Outdoors sheltered / cool bright indoors if very cold', 'Outdoors', 'Full sun', 'Let dry slightly; don''t stay soggy', NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'After flowering / spring', 'Trim tips; avoid hard cut into old wood', NULL, NULL, 'Unknown', NULL, 'Tender-ish; protect from severe frost', 'No/low (culinary)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Thyme', 'Thymus vulgaris', 'Herb (potted)', 'Outdoors (well-drained)', 'Outdoors', 'Full sun', 'Let dry; water sparingly', NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'Spring', 'Shear lightly to keep compact', NULL, NULL, 'Unknown', NULL, 'Hardy; drainage critical', 'No/low (culinary)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Lemon Thyme', 'Thymus × citriodorus', 'Herb (potted)', 'Outdoors (well-drained)', 'Outdoors', 'Full sun', 'Let dry; water sparingly', NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'Spring', 'Shear lightly to keep compact', NULL, NULL, 'Unknown', NULL, 'Hardy; drainage critical', 'No/low (culinary)', NULL, NULL, NULL
    UNION ALL
    SELECT 'Astrantia', 'Astrantia (masterwort)', 'Perennial (potted)', 'Outdoors', 'Outdoors', 'Part shade / morning sun', 'Keep evenly moist (more than herbs)', NULL, 'Medium', NULL, NULL, NULL, NULL, NULL, 'After flowering / autumn tidy', 'Deadhead to extend bloom; cut back spent stems', NULL, NULL, 'Unknown', NULL, 'Hardy; protect pot from severe freeze', 'Unknown', NULL, NULL, NULL
)
WHERE NOT EXISTS (SELECT 1 FROM plants);
