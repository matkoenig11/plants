import QtQuick 2.15
import QtTest 1.2
import "../../mobile_app/utils/CareFrequency.js" as CareFrequency

TestCase {
    name: "CareFrequency"

    function test_day_range() {
        const parsed = CareFrequency.parseWateringFrequency("5-7 days");
        verify(parsed.valid);
        compare(parsed.minDays, 5);
        compare(parsed.maxDays, 7);
    }

    function test_single_week() {
        const parsed = CareFrequency.parseWateringFrequency("1 week");
        verify(parsed.valid);
        compare(parsed.minDays, 7);
        compare(parsed.maxDays, 7);
    }

    function test_named_weekly_frequency() {
        const parsed = CareFrequency.parseWateringFrequency("twice a week");
        verify(parsed.valid);
        compare(parsed.minDays, 3);
        compare(parsed.maxDays, 4);
    }

    function test_embedded_weekly_range() {
        const parsed = CareFrequency.parseWateringFrequency("Keep lightly moist; 1-2x/week (adjust)");
        verify(parsed.valid);
        compare(parsed.minDays, 3);
        compare(parsed.maxDays, 7);
    }

    function test_invalid_text() {
        const parsed = CareFrequency.parseWateringFrequency("when the plant feels like it");
        verify(!parsed.valid);
    }
}
