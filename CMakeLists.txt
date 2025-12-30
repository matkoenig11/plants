cmake_minimum_required(VERSION 3.20)

project(PlantJournal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Sql Test QuickTest)

# Backend (shared between desktop and mobile)
add_library(PlantJournalBackend STATIC
    services/PlantListViewModel.cpp
    services/JournalEntryListViewModel.cpp
    services/ReminderListViewModel.cpp
    services/ReminderSettingsViewModel.cpp
    services/ReminderScheduler.cpp
    data/MigrationRunner.cpp
    data/PlantRepository.cpp
    data/JournalEntryRepository.cpp
    data/ReminderRepository.cpp
    data/ReminderSettingsRepository.cpp
)

target_include_directories(PlantJournalBackend PUBLIC
    core
    data
    services
)

target_link_libraries(PlantJournalBackend PUBLIC Qt6::Core Qt6::Sql)

# if(ANDROID)
if (true)  # Temporarily force mobile build for testing
    # Mobile app
    qt_add_executable(PlantJournalMobile
        mobile_app/main.cpp
    )

    qt_add_resources(PlantJournalMobile "mobile_qml"
        PREFIX "/"
        FILES
            mobile_app/Main.qml
            mobile_app/pages/PlantListPage.qml
            mobile_app/pages/PlantDetailPage.qml
            mobile_app/pages/PlantDetailPager.qml
    )

    qt_add_resources(PlantJournalMobile "mobile_migrations"
        PREFIX "/migrations"
        BASE "data/migrations"
        FILES
            data/migrations/001_init.sql
            data/migrations/002_add_plant_fields.sql
            data/migrations/003_reminders_multi.sql
            data/migrations/004_add_reminder_name.sql
            data/migrations/005_seed_plants.sql
    )

    target_link_libraries(PlantJournalMobile PRIVATE PlantJournalBackend Qt6::Quick Qt6::QuickControls2 Qt6::Sql)
else()
    # Desktop app
    qt_add_executable(PlantJournal
        app/main.cpp
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
            data/migrations/005_seed_plants.sql
    )

    target_link_libraries(PlantJournal PRIVATE PlantJournalBackend Qt6::Quick Qt6::QuickControls2 Qt6::Sql)
endif()

enable_testing()
add_subdirectory(tests)
