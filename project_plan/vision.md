# Vision

Plant Journal is a local-first desktop app for tracking plants, care instructions, care history, and reminders. It should feel fast and simple, keep data private, and work offline without dependencies beyond SQLite. Later it should interface and sync data with my private home server.

## Goals
- Track a personal plant collection with notes, locations, and acquisition details.
- Keep a calendar of care tasks and history, sync using google calendar.
- Show plant care instructions and schedules.
- Log care actions (water, prune, fertilize, repot, etc.) as journal entries.
- Provide reminders for upcoming care tasks.
- Keep UI separate from data access and business logic.
- Support batch care actions across multiple plants.

## Non-Goals (for MVP)
- Cloud sync or multi-user collaboration.
- Photo cataloging and image editing.
- Social sharing or public plant profiles.

## Principles
- Local-first: data lives on the device in SQLite.
- Simple and explicit code structure.
- QML for UI, C++ for logic and persistence.
- Respect system light/dark appearance or allow user toggle.
