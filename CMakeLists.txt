cmake_minimum_required(VERSION 3.20)

project(PlantJournal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Sql)

qt_add_executable(PlantJournal
    app/main.cpp
    services/PlantListViewModel.h
    services/PlantListViewModel.cpp
    services/JournalEntryListViewModel.h
    services/JournalEntryListViewModel.cpp
    services/ReminderListViewModel.h
    services/ReminderListViewModel.cpp
    services/ReminderSettingsViewModel.h
    services/ReminderSettingsViewModel.cpp
    services/ReminderScheduler.cpp
    data/MigrationRunner.cpp
    data/PlantRepository.cpp
    data/JournalEntryRepository.cpp
    data/ReminderRepository.cpp
    data/ReminderSettingsRepository.cpp
)

qt_add_resources(PlantJournal "qml"
    PREFIX "/"
    FILES
        app/Main.qml
        app/components/PlantListPane.qml
        app/components/PlantDetailPane.qml
        app/components/JournalEntryPane.qml
        app/components/BatchJournalPane.qml
        app/components/ReminderPane.qml
)

qt_add_resources(PlantJournal "migrations"
    PREFIX "/migrations"
    BASE "data/migrations"
    FILES
        data/migrations/001_init.sql
        data/migrations/002_add_plant_fields.sql
        data/migrations/003_reminders_multi.sql
        data/migrations/004_add_reminder_name.sql
)

target_include_directories(PlantJournal PRIVATE
    app
    core
    data
    services
)

target_link_libraries(PlantJournal PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Sql)
