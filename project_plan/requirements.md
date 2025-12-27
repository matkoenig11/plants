# Requirements

## Functional
- Maintain a plant database with name, species, location, acquisition date, and notes, and further attributes as needed.
- Log journal entries per plant (type, date, notes).
- Schedule reminders per plant (type, due date, completion state).
- Support CRUD operations for plants, entries, and reminders.
- Support batch logging for watering/fertilizing across multiple plants.

## Data
- Use SQLite as the storage backend.
- Use migrations for schema changes.
- Store dates as ISO 8601 text.
- Expand plant schema to include care instructions and environmental fields.

## Architecture
- QML for UI; no direct DB access from QML.
- C++ services/ViewModels expose data and commands to QML.
- Repositories handle persistence for each domain model.

## Non-Functional
- Local-first and offline by default.
- Fast startup and responsive UI.
- Simple, explicit code and project structure.
