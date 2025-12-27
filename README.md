# Plant Journal

Local-first desktop app for tracking plants, journal entries, and reminders.

## Build

- Configure: `cmake -S . -B build`
- Build: `cmake --build build`
- Run: `./build/PlantJournal`
- Tests: `ctest --test-dir build`

## Notes

- UI lives in `/app`.
- Business logic lives in `/services`.
- SQLite access and migrations live in `/data`.
- Domain models live in `/core`.
