cmake_minimum_required(VERSION 3.20)

project(PlantJournal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Sql Test QuickTest Network Multimedia)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "$ENV{QTDIR}/bin")

set(QT_SQL_DRIVER_DLLS)

if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    get_filename_component(QT_BIN_DIR "${WINDEPLOYQT_EXECUTABLE}" DIRECTORY)
    get_filename_component(QT_ROOT_DIR "${QT_BIN_DIR}" DIRECTORY)

    list(APPEND QT_SQL_DRIVER_DLLS
        "${QT_ROOT_DIR}/plugins/sqldrivers/qsqlite.dll"
        "${QT_ROOT_DIR}/plugins/sqldrivers/qsqlited.dll"
    )
endif()

function(deploy_sql_runtime target_name)
    if(NOT WIN32)
        return()
    endif()

    set(deploy_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/sqldrivers"
    )

    foreach(driver_dll IN LISTS QT_SQL_DRIVER_DLLS)
        if(EXISTS "${driver_dll}")
            list(APPEND deploy_commands
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${driver_dll}"
                        "$<TARGET_FILE_DIR:${target_name}>/sqldrivers/"
            )
        endif()
    endforeach()

    add_custom_command(TARGET ${target_name} POST_BUILD
        ${deploy_commands}
    )
endfunction()

# Backend (shared between desktop and mobile)
add_library(PlantJournalBackend STATIC
    core/Plant.cpp
    services/PlantListViewModel.cpp
    services/JournalEntryListViewModel.cpp
    services/ReminderListViewModel.cpp
    services/ReminderSettingsViewModel.cpp
    services/ReminderScheduler.cpp
    services/DatabaseConnectionViewModel.cpp
    services/PlantLibraryViewModel.cpp
    services/TagCatalogViewModel.cpp
    services/DatabaseSyncService.cpp
    data/MigrationRunner.cpp
    data/JournalEntryRepository.cpp
    data/ReminderRepository.cpp
    data/ReminderSettingsRepository.cpp
    data/database/SyncServerSettingsRepository.cpp
)

target_include_directories(PlantJournalBackend PUBLIC
    core
    data
    data/database
    services
)

target_link_libraries(PlantJournalBackend PUBLIC Qt6::Core Qt6::Sql Qt6::Network)

# if(ANDROID)
if (true)  # Temporarily force mobile build for testing
    # Mobile app
    qt_add_executable(PlantJournalMobile
        mobile_app/main.cpp
    )

    # Android packaging: point to custom manifest (permissions, etc.)
    # set_target_properties(PlantJournalMobile PROPERTIES
    #     ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android"
    # )
    set_property(TARGET PlantJournalMobile PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
          "${CMAKE_CURRENT_SOURCE_DIR}/android")

    qt_add_resources(PlantJournalMobile "mobile_qml"
        PREFIX "/"
        FILES
            ui/AppConstants.qml
            mobile_app/Main.qml
            mobile_app/pages/PlantListPage.qml
            mobile_app/pages/JournalPage.qml
            mobile_app/pages/PlantDetailPager.qml
            mobile_app/pages/PlantDetailEditPage.qml
            mobile_app/pages/AddPlantLibraryPage.qml
            mobile_app/pages/CameraCapturePage.qml
            mobile_app/pages/DatabaseConnectionPage.qml
            mobile_app/pages/LibraryPage.qml
            mobile_app/pages/TagManagementPage.qml
            mobile_app/utils/CareFrequency.js
            mobile_app/pages/details/PlantDetailPage.qml
            mobile_app/pages/details/GeneralInfoSection.qml
            mobile_app/pages/details/GeneralCareSection.qml
            mobile_app/pages/details/UpdateSection.qml
    )

    qt_add_resources(PlantJournalMobile "mobile_sql"
        PREFIX "/sql"
        BASE "data/sql"
        FILES
            data/sql/schema/001_plants.sql
            data/sql/schema/002_journal_entries.sql
            data/sql/schema/003_reminders.sql
            data/sql/plants/101_identity_and_location.sql
            data/sql/plants/102_care_profile.sql
            data/sql/plants/103_health_and_source.sql
            data/sql/plants/104_plant_images.sql
            data/sql/plants/105_remove_unused_fields.sql
            data/sql/plants/106_library_fields.sql
            data/sql/plants/107_tags.sql
            data/sql/plants/108_tag_catalog.sql
            data/sql/schedules/001_plant_care_schedules.sql
            data/sql/reminders/101_settings.sql
            data/sql/reminders/102_multi_plant_rework.sql
            data/sql/reminders/201_custom_name.sql
            data/sql/seeds/001_plants.sql
            data/sql/sync/001_sync_metadata.sql
            data/sql/sync/002_tombstone_deletion_timestamps.sql
            data/sql/sync/003_tag_catalog_sync.sql
    )

    target_link_libraries(PlantJournalMobile PRIVATE PlantJournalBackend Qt6::Quick Qt6::QuickControls2 Qt6::Sql Qt6::Multimedia)
    deploy_sql_runtime(PlantJournalMobile)
else()
    # Desktop app
    qt_add_executable(PlantJournal
        app/main.cpp
    )

    qt_add_resources(PlantJournal "qml"
        PREFIX "/"
        FILES
            ui/AppConstants.qml
            app/Main.qml
            app/components/PlantListPane.qml
            app/components/PlantDetailPane.qml
            app/components/JournalEntryPane.qml
            app/components/BatchJournalPane.qml
            app/components/ReminderPane.qml
    )

    qt_add_resources(PlantJournal "sql"
        PREFIX "/sql"
        BASE "data/sql"
        FILES
            data/sql/schema/001_plants.sql
            data/sql/schema/002_journal_entries.sql
            data/sql/schema/003_reminders.sql
            data/sql/plants/101_identity_and_location.sql
            data/sql/plants/102_care_profile.sql
            data/sql/plants/103_health_and_source.sql
            data/sql/plants/104_plant_images.sql
            data/sql/plants/105_remove_unused_fields.sql
            data/sql/plants/106_library_fields.sql
            data/sql/plants/107_tags.sql
            data/sql/plants/108_tag_catalog.sql
            data/sql/schedules/001_plant_care_schedules.sql
            data/sql/reminders/101_settings.sql
            data/sql/reminders/102_multi_plant_rework.sql
            data/sql/reminders/201_custom_name.sql
            data/sql/seeds/001_plants.sql
            data/sql/sync/001_sync_metadata.sql
            data/sql/sync/002_tombstone_deletion_timestamps.sql
            data/sql/sync/003_tag_catalog_sync.sql
    )

    target_link_libraries(PlantJournal PRIVATE PlantJournalBackend Qt6::Quick Qt6::QuickControls2 Qt6::Sql)
    deploy_sql_runtime(PlantJournal)
endif()

enable_testing()
add_subdirectory(tests)
