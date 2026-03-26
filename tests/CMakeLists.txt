add_executable(ReminderSchedulerTests
    core/test_reminder_scheduler.cpp
)

target_link_libraries(ReminderSchedulerTests PRIVATE PlantJournalBackend Qt6::Test)

add_test(NAME ReminderSchedulerTests COMMAND ReminderSchedulerTests)

add_executable(PlantListViewModelTests
    services/test_plant_list_view_model.cpp
)

target_link_libraries(PlantListViewModelTests PRIVATE PlantJournalBackend Qt6::Test Qt6::Sql)

add_test(NAME PlantListViewModelTests COMMAND PlantListViewModelTests)

add_executable(ReminderRepositoryTests
    data/test_reminder_repository.cpp
)

target_link_libraries(ReminderRepositoryTests PRIVATE PlantJournalBackend Qt6::Test Qt6::Sql)

add_test(NAME ReminderRepositoryTests COMMAND ReminderRepositoryTests)

add_executable(MigrationRunnerTests
    data/test_migration_runner.cpp
)

qt_add_resources(MigrationRunnerTests "migration_sql"
    PREFIX "/sql"
    BASE "${CMAKE_SOURCE_DIR}/data/sql"
    FILES
        "${CMAKE_SOURCE_DIR}/data/sql/schema/001_plants.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/schema/002_journal_entries.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/schema/003_reminders.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/101_identity_and_location.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/102_care_profile.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/103_health_and_source.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/104_plant_images.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/105_remove_unused_fields.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/106_library_fields.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/schedules/001_plant_care_schedules.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/reminders/101_settings.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/reminders/102_multi_plant_rework.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/reminders/201_custom_name.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/seeds/001_plants.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/sync/001_sync_metadata.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/sync/002_tombstone_deletion_timestamps.sql"
)

target_link_libraries(MigrationRunnerTests PRIVATE PlantJournalBackend Qt6::Test Qt6::Sql)

add_test(NAME MigrationRunnerTests COMMAND MigrationRunnerTests)

add_executable(DatabaseSyncServiceTests
    services/test_database_sync_service.cpp
)

qt_add_resources(DatabaseSyncServiceTests "database_sync_sql"
    PREFIX "/sql"
    BASE "${CMAKE_SOURCE_DIR}/data/sql"
    FILES
        "${CMAKE_SOURCE_DIR}/data/sql/schema/001_plants.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/schema/002_journal_entries.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/schema/003_reminders.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/101_identity_and_location.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/102_care_profile.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/103_health_and_source.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/104_plant_images.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/105_remove_unused_fields.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/plants/106_library_fields.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/schedules/001_plant_care_schedules.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/reminders/101_settings.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/reminders/102_multi_plant_rework.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/reminders/201_custom_name.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/seeds/001_plants.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/sync/001_sync_metadata.sql"
        "${CMAKE_SOURCE_DIR}/data/sql/sync/002_tombstone_deletion_timestamps.sql"
)

target_link_libraries(DatabaseSyncServiceTests PRIVATE PlantJournalBackend Qt6::Test Qt6::Sql Qt6::Network)

add_test(NAME DatabaseSyncServiceTests COMMAND DatabaseSyncServiceTests)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(NAME SeedPopulationPython
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/check_seed.py ${CMAKE_SOURCE_DIR}
)

add_test(NAME GardenApiPythonTests
    COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_SOURCE_DIR}/tests/API -p "test_*.py"
)

# QML tests
qt_add_executable(QmlTests
    qml/qmltest_main.cpp
)

target_link_libraries(QmlTests PRIVATE Qt6::QuickTest)

add_test(NAME QmlTests COMMAND QmlTests)
set_tests_properties(QmlTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/qml
)
