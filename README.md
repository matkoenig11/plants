# Plant Journal

Local-first desktop app for tracking plants, journal entries, and reminders.

## Build

- Configure: `cmake -S . -B build`
- Build: `cmake --build build`
- Run: `./build/PlantJournal`
- Tests: `ctest --test-dir build`

## Mobile sample app

- A minimal toggle demo lives in `/mobile_app` (label shows Off/On when pressing the button).
- Build it: `cmake -S . -B build` then `cmake --build build --target PlantJournalMobile` (add `--config Release` on multi-config generators like MSVC).
- Run it on desktop as a sanity check: `./build/PlantJournalMobile` (or `build-win/Release/PlantJournalMobile.exe` on Windows).

## Mobile setup checklist

- Install Qt with the Android and/or iOS kits (Qt Maintenance Tool or Qt online installer).
- Android: install Android SDK + NDK (matching Qt docs), set `ANDROID_SDK_ROOT`, `ANDROID_NDK_ROOT`, and Java/JDK (`JAVA_HOME`). Use the Qt for Android CMake toolchain (or Qt Creator Android kit) to configure, then package with `androiddeployqt` (handled automatically by Qt Creator).
- iOS: install Xcode + command-line tools, accept licenses, and use the Qt for iOS kit. Provisioning profile and signing certificate are required; Qt Creator will prompt/configure them. Deploy via Xcode or `ios-deploy` as guided by Qt Creator.
- Use Qt Creator kits to pick the mobile target and run/deploy, or invoke CMake manually with the platform toolchain supplied by Qt (e.g., `-DCMAKE_TOOLCHAIN_FILE=<Qt>/.../android.toolchain.cmake`).
- After building, run on a device/emulator from Qt Creator to verify the QML UI launches (the toggle demo is a quick validation target).
## Notes

- UI lives in `/app`.
- Business logic lives in `/services`.
- SQLite access and migrations live in `/data`.
- Domain models live in `/core`.
