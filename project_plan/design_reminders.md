# Reminder System UX — Design Notes

## Core Idea
Reminders can be assigned to one or many plants and a task type (water/fertilize/prune/repot/custom). Delivery timing is configured globally (day before, day of, overdue follow‑ups) so the user sets it once.

## Reminder Model (Single or Multi-Plant)
- Task type: Water / Fertilize / Prune / Repot / Custom
- Schedule: interval-based (e.g., every 7 days) or fixed date
- Start date (or last-done date)
- Next due date (computed)
- Optional notes
- Plants: one or many associated plants per reminder

## Global Delivery Settings
- Day before: toggle + time (e.g., 18:00)
- Day of: toggle + time (e.g., 08:00)
- Overdue follow-up: toggle + cadence (e.g., every 2 days at 09:00)

## Suggested UX

### Plant Details → Reminders Section
- Simple form:
  - Task type
  - Interval (days) or fixed date
  - Start date / last done
  - Toggle: “Apply to multiple plants”
  - Plant selector (multi-select list)
- Preview: “Next due: Tue 9 Jan” + “Applies to: Monstera, Pothos”

### Reminder List View
- Tabs or sections:
  - Today
  - Upcoming
  - Overdue
- Each reminder shows plant name, task, due date, and quick actions.

### Actions
- Mark done (updates last done + recalculates next due)
- Snooze (1 day / 3 days / custom)

## Optional Enhancements
- Auto-create reminders after logging an action
- Per-task delivery time override

## Open Questions
1) Should reminders be only per plant+task, or allow custom one-off reminders? Yes, reminders should be per plant+task.
2) Are delivery times global or per task? Yes delivery times are global.
3) Should overdue follow-ups be daily or configurable? Yes, configurable.
4) Auto-create reminders on journal entries? Yes.
