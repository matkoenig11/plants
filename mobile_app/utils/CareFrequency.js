.pragma library

function invalidRange() {
    return {
        valid: false,
        minDays: 0,
        maxDays: 0,
        description: ""
    };
}

function validRange(minDays, maxDays) {
    const lower = Math.max(1, Math.min(minDays, maxDays));
    const upper = Math.max(lower, Math.max(minDays, maxDays));
    return {
        valid: true,
        minDays: lower,
        maxDays: upper,
        description: formatRange(lower, upper)
    };
}

function formatRange(minDays, maxDays) {
    if (minDays === maxDays) {
        return minDays === 1 ? "1 day" : (String(minDays) + " days");
    }
    return String(minDays) + "-" + String(maxDays) + " days";
}

function normalize(text) {
    return String(text || "")
        .toLowerCase()
        .replace(/[(),;]/g, " ")
        .replace(/\s+/g, " ")
        .trim();
}

function countFromToken(token) {
    switch (token) {
    case "once":
        return 1;
    case "twice":
        return 2;
    case "thrice":
        return 3;
    default: {
        const parsed = parseInt(token, 10);
        return isNaN(parsed) ? 0 : parsed;
    }
    }
}

function weeklyRange(minPerWeek, maxPerWeek) {
    if (minPerWeek <= 0 || maxPerWeek <= 0) {
        return invalidRange();
    }

    const lowerCount = Math.min(minPerWeek, maxPerWeek);
    const upperCount = Math.max(minPerWeek, maxPerWeek);
    const minDays = Math.max(1, Math.floor(7 / upperCount));
    const maxDays = Math.max(minDays, Math.ceil(7 / lowerCount));
    return validRange(minDays, maxDays);
}

function parseWateringFrequency(text) {
    const normalized = normalize(text);
    if (!normalized.length) {
        return invalidRange();
    }

    let match = null;

    if (/\bdaily\b/.test(normalized) || /\bevery day\b/.test(normalized)) {
        return validRange(1, 1);
    }

    if (/\bevery other day\b/.test(normalized)) {
        return validRange(2, 2);
    }

    if (/\bweekly\b/.test(normalized)) {
        return validRange(7, 7);
    }

    if (/\bfortnightly\b/.test(normalized)) {
        return validRange(14, 14);
    }

    match = normalized.match(/\b(?:every\s+)?(\d+)\s*(?:-|to)\s*(\d+)\s*days?\b/);
    if (match) {
        return validRange(parseInt(match[1], 10), parseInt(match[2], 10));
    }

    match = normalized.match(/\b(?:every\s+)?(\d+)\s*days?\b/);
    if (match) {
        const days = parseInt(match[1], 10);
        return validRange(days, days);
    }

    match = normalized.match(/\b(?:every\s+)?(\d+)\s*(?:-|to)\s*(\d+)\s*weeks?\b/);
    if (match) {
        return validRange(parseInt(match[1], 10) * 7, parseInt(match[2], 10) * 7);
    }

    match = normalized.match(/\b(?:every\s+)?(\d+)\s*weeks?\b/);
    if (match) {
        const days = parseInt(match[1], 10) * 7;
        return validRange(days, days);
    }

    match = normalized.match(/\b(\d+)\s*(?:-|to)\s*(\d+)\s*x\s*\/\s*week\b/);
    if (match) {
        return weeklyRange(parseInt(match[1], 10), parseInt(match[2], 10));
    }

    match = normalized.match(/\b(\d+)\s*x\s*\/\s*week\b/);
    if (match) {
        const count = parseInt(match[1], 10);
        return weeklyRange(count, count);
    }

    match = normalized.match(/\b(\d+)\s*(?:-|to)\s*(\d+)\s*times?\s*(?:a|per)\s*week\b/);
    if (match) {
        return weeklyRange(parseInt(match[1], 10), parseInt(match[2], 10));
    }

    match = normalized.match(/\b(\d+)\s*times?\s*(?:a|per)\s*week\b/);
    if (match) {
        const count = parseInt(match[1], 10);
        return weeklyRange(count, count);
    }

    match = normalized.match(/\b(once|twice|thrice)\s+a\s+week\b/);
    if (match) {
        const count = countFromToken(match[1]);
        return weeklyRange(count, count);
    }

    match = normalized.match(/\b(once|twice|thrice)\s+weekly\b/);
    if (match) {
        const count = countFromToken(match[1]);
        return weeklyRange(count, count);
    }

    return invalidRange();
}
