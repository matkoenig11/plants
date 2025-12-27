# AGENTS.md — Plant Journal (Qt/QML + C++)

## Goal
Build a local-first desktop app:
- Plant database
- Journal entries (water/prune/fertilize/repot/etc.)
- Reminders (next water/prune) + notifications
Keep UI (QML) separate from backend (C++ services + SQLite).

## Tech
- Qt 6 (CMake project)
- QML for UI
- C++ for business logic
- SQLite for storage (via Qt SQL module OR a thin sqlite wrapper)
- Qt Test + Qt Quick Test for unit and QML tests

## Repo layout
- /app            QML + minimal glue
- /core           domain models (Plant, JournalEntry, Reminder)
- /data           SQLite access, migrations, repositories
- /services       business logic (scheduler, recommendation rules)
- /tests          unit tests
- /project_plan   vision, requirements, roadmap, planning docs

## Commands (fill in once created)
- Configure: `cmake -S . -B build`
- Build: `cmake --build build`
- Run: `./build/PlantJournal` (or platform equivalent)
- Tests: `ctest --test-dir build`

## Coding rules
- No DB access from QML.
- QML talks only to C++ ViewModels/Controllers exposed to QML.
- Use repositories for all persistence.
- Always add/adjust a migration when changing schema.
- Prefer simple, explicit code over frameworks.

## Current priorities
1) SQLite schema + repositories
2) CRUD Plants
3) Journal entry logging
4) Reminder scheduling + notifications

## Testing strategy
- Unit tests: repositories, ViewModels, scheduler logic (Qt Test).
- QML tests: basic component checks (Qt Quick Test).
- Integration tests: repo + migrations + ViewModel flows.
- CI: build + ctest, use offscreen QML where needed.

# Initial data source
The inital data can be found in the plants_populated.xlsx file. 
