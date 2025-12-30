# Plant Journal — Mobile Design Notes

## Navigation
- Bottom nav (4 tabs): Plants, Journal, Reminders, Plan.
- FAB per tab for the primary action (add plant / log entry / add reminder / add task).

## Screens
- Plants: card list (name, status chip “Water today”, last watered). Tap → Plant detail.
- Plant detail: hero image placeholder, key facts (type, light, water cadence), quick actions (Water, Prune, Fertilize), recent journal list.
- Journal: reverse-chron list (action icon, plant name, note snippet, timestamp). Filter chips: All / Water / Prune / Fertilize / Repot.
- Reminders: grouped by due (Today / Upcoming), switches to enable/disable, snooze/mark done.
- Plan: weekly agenda merging reminders + tasks. Day strip + list with completion toggles.

## Plan Page Layout (responsive)
- Header: “This week” + Today pill.
- Day selector: horizontal chips for 7 days (Today highlighted).
- Task list: cards “Plant · Action” + due time; toggle/checkbox to complete; overdue red, today accent, future muted.
- Empty state: “No tasks today. Add one?” with emoji/illustration.
- FAB: “Add task/reminder” (pre-fills selected day).

## Components
- Cards with light elevation, rounded corners.
- Chips for filters and day selector.
- Bottom nav anchored; FAB bottom-right.
- Accent: fresh green; background off-white; secondary text gray.

## Data per page
- Plants: name, type, next water date, last watered, health chip/emoji.
- Journal: action type, plant, note snippet, timestamp.
- Reminders: due date/time, plant, action type, enable switch, kebab (edit/delete).
- Plan: merged items (reminders/tasks) with status; completed gray/crossed.

## Flows
- Add plant: Plants FAB → form (name required).
- Log entry: Journal FAB or quick action on Plant detail.
- Add reminder/task: Reminders or Plan FAB → select plant, action, cadence/date.

## Technical notes for mobile build
- Android/iOS kits in Qt Creator; build target is `PlantJournalMobile` (mobile_app/Main.qml).
- For Android: set JAVA_HOME, ANDROID_SDK_ROOT, ANDROID_NDK_ROOT; use Qt’s Android kit and deploy from Qt Creator to device/emulator.
- For iOS: use Qt iOS kit with Xcode + provisioning profile; deploy from Qt Creator. 

## References
- KDAB: Qt Allstack Part 1 (setup + stack choices) — https://www.kdab.com/qt-allstack-part-1-setup/
- Qt Docs: Qt Quick Controls 2 overview (component gallery, mobile-friendly controls) — https://doc.qt.io/qt-6/qtquickcontrols2-index.html
- Qt Docs: Material style for Qt Quick Controls 2 (mobile visual language) — https://doc.qt.io/qt-6/qtquickcontrols2-material.html
- Qt Docs: SwipeView control (horizontal page navigation) — https://doc.qt.io/qt-6/qml-qtquick-controls2-swipeview.html
- QML Book: UI controls and layout patterns (practical QML examples) — https://qmlbook.github.io/
- Qt Docs: UX guidelines for Qt Quick Controls 2 (navigation patterns, touch targets, transitions) � https://doc.qt.io/qt-6/qtquickcontrols2-ux-guidelines.html
- Qt Docs: Qt Quick Controls 2 gallery example (full control showcase for mobile/desktop) � https://doc.qt.io/qt-6/qtquickcontrols-gallery-example.html
- Qt Docs: Imagine style for Qt Quick Controls 2 (skinning for custom mobile looks) � https://doc.qt.io/qt-6/qtquickcontrols2-imagine.html
- Qt Blog: Designing fluid UIs with Qt Quick Controls 2 (performance + animation tips) � https://www.qt.io/blog/designing-fluid-uis-with-qt-quick-controls-2
