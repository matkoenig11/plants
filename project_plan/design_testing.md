# Testing + CI/CD Strategy (Qt/QML)

## References (Qt docs)
- Qt Test module: provides C++ API for unit testing and benchmarking Qt applications/libraries.
- Qt Quick Test module: unit testing framework for QML.

## Testing Goals
- Catch regressions in data layer (repositories/migrations).
- Validate business logic (schedulers, validation rules).
- Ensure QML/ViewModels stay in sync.
- Keep tests fast enough for local runs and CI.

## Test Pyramid for This App

### 1) Unit Tests (fast, most coverage)
Focus on C++ logic and data mapping.

Recommended units:
- Repositories: PlantRepository, JournalEntryRepository, ReminderRepository
  - CRUD happy path and error cases.
  - Schema migrations applied correctly.
  - Join tables (reminder_plants) handled correctly.
- ViewModels:
  - Validation (date parsing, required fields).
  - Role mapping and dataChanged notifications.
- Scheduler logic:
  - ReminderScheduler due calculations (day before, day of, overdue cadence).

Framework:
- Qt Test (Qt::Test)
- Use in-memory SQLite ("QSQLITE" with ":memory:") for fast tests.

### 2) QML Unit Tests (logic + bindings)
Use Qt Quick Test for QML components with minimal UI.

Targets:
- QML components render with default states (no crashes).
- Reminders pane toggles: schedule type -> correct fields enabled.
- Theme toggle updates Material theme.

### 3) Integration Tests (DB + ViewModel + QML)
- Spin up SQLite file per test, apply migrations, then run repo + ViewModel workflows.
- Validate full flow: create plant -> add reminder -> list shows expected data.
- Keep count small to avoid slow CI.

### 4) Manual/Exploratory Testing (smoke)
- Launch app, create/edit/delete plants, journal entries, reminders.
- Verify theme toggle and persistence behavior.
- Batch actions (watering/fertilizing) end-to-end.

## Test Structure Proposal
- /tests
  - core/ (pure logic)
  - data/ (repositories + migrations)
  - services/ (ViewModels + scheduler)
  - qml/ (Qt Quick Test)

## CMake + Qt Test Wiring
- Add Qt6::Test dependency for C++ unit tests.
- Use qt_add_executable for test binaries or add_library + add_executable.
- Register tests with CTest (add_test).
- For QML tests, use Qt Quick Test runner and include QML test files in /tests/qml.

## CI/CD Strategy (GitHub Actions)

### Build + Test (every push/PR)
- Matrix: Ubuntu (primary), optionally macOS/Windows later.
- Steps:
  1) Install Qt (using aqtinstall or cached Qt toolchain).
  2) Configure: `cmake -S . -B build`.
  3) Build: `cmake --build build`.
  4) Test: `ctest --test-dir build --output-on-failure`.

### Artifacts (optional)
- Upload build artifacts for manual download on tagged releases.

### Release Pipeline (optional)
- On tag: build release binaries, attach to GitHub Releases.

## CI Notes
- QML tests may require offscreen platform; use `QT_QPA_PLATFORM=offscreen`.
- Avoid network in tests unless explicitly required.
- Keep DB tests isolated (temp dirs, clean up files).

## Next Steps (Implementation)
1) Add Qt Test dependency + a first C++ test (e.g., ReminderScheduler).
2) Add a repository test with in-memory SQLite.
3) Add minimal QML test using Qt Quick Test.
4) Add GitHub Actions workflow for build + tests.
